#include "EventLoop.h"
#include "CCEpoll.h"
#include "CCSystem.h"
#include "CCThread.h"

CTL::EventLoop::EventLoop() : epoll_(), running(false), event_() {
    epoll_.Create(0);
    // 初始化时间轮
    wheel_.resize(WHEEL_SIZE);
    current_tick_ = 0;
    listen_fd = INVALID_SOCKET;
}

CTL::EventLoop::~EventLoop() {
    stop();
    epoll_.Close();
}

void CTL::EventLoop::registerEvent(const SOCKET fd, const Callback &cb,const uint32_t event_t) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto event = new Event_t;
    event->cb = cb;
    event->event = event_t;
    callbacks_.put(fd,event);
    struct epoll_event ev = {};
    ev.events = event_t;
    ev.data.fd = fd;
    epoll_.Epoll_CTL(Socket(fd), &ev, CTL_ADD);
}

void CTL::EventLoop::unregisterEvent(const SOCKET fd) {
    std::lock_guard<std::mutex> lock(mutex_);
    // 从回调映射中删除
    callbacks_.Remove(fd);
    auto event = callbacks_.get(fd);
    event = nullptr;
    // 从epoll中移除监听
    struct epoll_event ev{};
    epoll_.Epoll_CTL(Socket(fd), &ev, CTL_DEL);
}

void CTL::EventLoop::addTimerAfter(const std::chrono::milliseconds delay, const Callback &cb, const bool repeat) {
    const auto expiration = std::chrono::steady_clock::now() + delay;
    addTimer(expiration, cb, repeat, delay);
}

void CTL::EventLoop::addTimerAt(const TimePoint expiration, const Callback &cb, const bool repeat) {
    addTimer(expiration, cb, repeat, std::chrono::milliseconds{0});
}

void CTL::EventLoop::run() {
    running = true;
    while (running) {
        // 处理定时任务
        processTimers();

        // 处理IO事件
        epoll_event events[1024] = { 0 };
        const int num = epoll_.Epoll_Wait(events,1024, 100); // 100ms超时

        if (num == -1) {
            if (errno == EINTR) {
                // 被信号中断，继续循环
                continue;
            }
            // 注意：这里需要根据平台处理错误情况
            CTL::System::Println("Epoll error: " + std::to_string(errno));
            break;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (num < 0) {
                continue;
            }
            for (int i = 0; i < num; ++i) {
                const int fd = events[i].data.fd;
                if (callbacks_.IsContains(fd)) {
                    const auto event_t = callbacks_.get(fd);
                    if (event_t && *event_t) {
                        const auto event = *event_t;
                        try {
                            if (fd == listen_fd) {
                                event->cb(EPEvent::CONNECT);
                            }
                            else if (events[i].events & EPOLLIN) {
                                event->cb(EPEvent::IN_t);
                            }
                            else if (events[i].events & EPOLLOUT) {
                                event->cb(EPEvent::OUT_t);
                            }
                            else if (events[i].events & EPOLLMSG) {
                                event->cb(EPEvent::MSG);
                            }
                            else if (events[i].events & EPOLLRDHUP) {
                                event->cb(EPEvent::RD_HUP);
                            }
                            else if (events[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
                                event->cb(EPEvent::CLOSE);
                            }
                        }
                        catch (CCException& e) {
                            CTL::System::Println("CTL::EventLoop::run Error: {}",e.what());
                        }
                    }
                }
            }
        }
    }
}

void CTL::EventLoop::stop() {
    if (running) {
        running = false;
        // 注意：原代码中尝试从epoll中删除自身，这可能不适用于封装后的实现
    }
}

void CTL::EventLoop::setListen(const SOCKET fd) {
    listen_fd = fd;
}

void CTL::EventLoop::addTimer(TimePoint expiration, Callback cb, bool repeat, std::chrono::milliseconds interval) {
    std::lock_guard<std::mutex> lock(mutex_);
    wheel_[getCurrentTick()].emplace(expiration, cb, repeat, interval);
}

void CTL::EventLoop::processTimers() {
    const auto now = std::chrono::steady_clock::now();
    const size_t ticks_to_process =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick_).count() / WHEEL_INTERVAL;

    for (size_t i = 0; i < ticks_to_process; ++i) {
        advanceWheel();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto& bucket = wheel_[current_tick_ % WHEEL_SIZE];
    while (!bucket.empty() && bucket.top().expiration <= now) {
        auto task = bucket.top();
        bucket.pop();
        {
            std::lock_guard<std::mutex> cb_lock(mutex_);
            const auto event = new Event_t;
            event->cb = task.cb;
            event->event = EPOLLIN | EPOLLOUT;
            callbacks_.put(next_timer_id_,event);
        }
        // 触发epoll事件
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLOUT;
        ev.data.fd = next_timer_id_;
        epoll_.Epoll_CTL(Socket(next_timer_id_), &ev, CTL_ADD);
        next_timer_id_++;
    }
}

void CTL::EventLoop::advanceWheel() {
    current_tick_++;
    last_tick_ = std::chrono::steady_clock::now();
    auto& bucket = wheel_[current_tick_ % WHEEL_SIZE];
    while (!bucket.empty()) {
        bucket.pop(); // 清空当前 tick 的所有定时器条目
    }
}

size_t CTL::EventLoop::getCurrentTick() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_tick_).count() / WHEEL_INTERVAL;
}
