#ifndef EPOLL_EVENTLOOP_H
#define EPOLL_EVENTLOOP_H

#include "CCEpoll.h"
#ifdef __linux__
#include <unistd.h>
#endif
#include <functional>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <iostream>
#include <chrono>
#include <set>
#include "TL/Map.h"

namespace CTL {
    enum EPEvent {
        None = 99999,
        IN_t = 1,
        OUT_t = 2,
        MSG = 4,
        RD_HUP = 5,
        CLOSE = 6,
        CONNECT = 111111,
    };
    class EventLoop {
        SOCKET listen_fd;
public:
    using Callback = std::function<void(uint32_t)>;
    using TimePoint = std::chrono::steady_clock::time_point;
    struct Event_t {
        Callback cb = nullptr;
        uint32_t event{};
    };
    EventLoop();
    ~EventLoop();

    // 注册IO事件
    void registerEvent(SOCKET fd, const Callback &cb,uint32_t event_t = EPOLLIN);

    // 移除IO事件
    void unregisterEvent(SOCKET fd);

    // 添加定时任务（延迟执行）
    void addTimerAfter(std::chrono::milliseconds delay, const Callback &cb, bool repeat = false);

    // 添加定时任务（指定时间点执行）
    void addTimerAt(TimePoint expiration, const Callback &cb, bool repeat = false);

    // 启动事件循环
    void run();

    // 停止事件循环
    void stop();

    void setListen(SOCKET fd);

private:
    struct TimerTask {
        TimePoint expiration;
        Callback cb;
        bool repeat;
        std::chrono::milliseconds interval;

        TimerTask(TimePoint exp, Callback callback, bool rep, std::chrono::milliseconds intv)
            : expiration(exp), cb(std::move(callback)), repeat(rep), interval(intv) {}

        // 新增部分：用于优先队列排序（按时间从小到大）
        bool operator<(const TimerTask& other) const {
            return expiration > other.expiration; // 小顶堆
        }
    };

    using TimerQueue = std::priority_queue<TimerTask, std::vector<TimerTask>, std::greater<TimerTask>>;

    void addTimer(TimePoint expiration, Callback cb, bool repeat, std::chrono::milliseconds interval);

    void processTimers();

    void advanceWheel();

    size_t getCurrentTick() const;

    static constexpr size_t WHEEL_SIZE = 60;      // 时间轮槽数
    static constexpr int WHEEL_INTERVAL = 1000;   // 每个槽的时间间隔（ms）

    Epoll epoll_{};
    Map<SOCKET, Event_t*> callbacks_;
    std::mutex mutex_;
    std::atomic<bool> running;

    // 时间轮实现
    std::vector<std::priority_queue<TimerTask>> wheel_;
    size_t current_tick_ = 0;
    TimePoint last_tick_ = std::chrono::steady_clock::now();

    // 用于定时器触发的epoll事件
    struct epoll_event event_;
        int next_timer_id_ = 0;
    };
}


#endif