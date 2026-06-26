#ifndef CC_TIMER_H
#define CC_TIMER_H
#pragma once

#include <CCThread.h>
#include "CC.h"
#include "CCPrecisionClock"

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#endif

#if defined(__linux__) || defined(__APPLE__) || defined(__OHOS__)
#include <ctime>
#include <csignal>
#include <sys/timerfd.h>
#include <unistd.h>
#include <poll.h>
#endif



namespace CTL {
    class ClockTimer{
        size_t last_time{};
        size_t last_duration = 10;
        long long deviation = {};
        size_t sleep_time_ms = 100;
        static size_t initAccuracy;
        bool ImprovePrecision = true;
    public:
        static void initializationAccuracy(const size_t ms){
            initAccuracy = ms;
#ifdef _WIN32
            ::timeBeginPeriod(ms);
#endif
        }
        static void finalizationAccuracy(){
#ifdef _WIN32
            ::timeEndPeriod(initAccuracy);
#endif
        }
        size_t sleepMS(const size_t ms){
            last_time = ms;
            using namespace std::chrono;
            const auto start = high_resolution_clock::now();
            const auto target = start + milliseconds(ms) - milliseconds(deviation);
            const auto max_wait = start + milliseconds(ms) - milliseconds(deviation + 5);
            if (ms > last_duration) {
                const auto a = last_duration / 3;
                const long long b = static_cast<long long>(ms) -
                    static_cast<long long>(a) - static_cast<long long>(deviation);
//                CTL::System::Println("sleepMS calculation: b = {} ({} - {} - {})", b, ms, a, deviation);
                if (b > 0) {
//                    CTL::System::Println("Calling SleepMS with {} ms", static_cast<unsigned int>(b));
                    CTL::Thread::SleepMS(static_cast<unsigned int>(b));
                }
            }
            while (high_resolution_clock::now() < target) {
                if (!ImprovePrecision) {
                    CTL::Thread::Sleep(1);
                }

                if (high_resolution_clock::now() > max_wait) {
                    break;
                }
            }
            const auto end = high_resolution_clock::now();
            const long long actual_duration = duration_cast<milliseconds>(end - start).count();
            deviation = actual_duration - ms;
//            CTL::System::Println("actual_duration {} ms", actual_duration);
            return actual_duration;
        }
        void resetDeviation(){
            deviation = 0;
        }
        void improvePrecision(const bool Flag){
            ImprovePrecision = Flag;
        }
    };
    enum TimeType {
        Millisecond = 1,
        Second = 2,
        Minute = 3,
        Hour = 4,
    };
#if defined(__linux__) || defined(__APPLE__) || defined(__OHOS__)
    class LinuxHighPrecisionTimer {
        int timer_fd;
        struct itimerspec timer_spec = {};
        std::function<void()> callback_func;
        std::atomic<bool> running;
        Thread worker_thread;
    public:
        LinuxHighPrecisionTimer() : timer_fd(-1), running(false) {}
        ~LinuxHighPrecisionTimer() {
            stop();
            if (timer_fd >= 0) {
                close(timer_fd);
            }
        }

        bool start(const std::function<void()>& callback,
                   const timespec& interval,
                   const timespec& initial_delay = {}) {
            if (running.load()) {
                return false;
            }

            // 创建 timerfd
            timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
            if (timer_fd < 0) {
                perror("timerfd_create failed");
                return false;
            }

            callback_func = callback;
            running.store(true);

            // 设置定时器参数
            timer_spec.it_value = initial_delay;
            if (timer_spec.it_value.tv_sec == 0 && timer_spec.it_value.tv_nsec == 0) {
                timer_spec.it_value = interval; // 如果没有初始延迟，直接使用间隔
            }
            timer_spec.it_interval = interval;

            // 启动定时器
            if (timerfd_settime(timer_fd, 0, &timer_spec, nullptr) < 0) {
                perror("timerfd_settime failed");
                close(timer_fd);
                timer_fd = -1;
                running.store(false);
                return false;
            }

            // 启动工作线程
            worker_thread.SetThread([this]() {
                pollfd pfd = {timer_fd, POLLIN, 0};
                uint64_t expirations;

                while (running.load()) {
                    int ret = poll(&pfd, 1, 100); // 100ms 超时

                    if (ret > 0 && (pfd.revents & POLLIN)) {
                        ssize_t s = read(timer_fd, &expirations, sizeof(uint64_t));
                        if (s == sizeof(uint64_t)) {
                            try {
                                if (callback_func) {
                                    callback_func();
                                }
                            } catch (const std::exception& e) {
                                System::Println("Timer callback exception: {}", e.what());
                            }
                        }
                    } else if (ret < 0 && errno != EINTR) {
                        perror("poll failed");
                        break;
                    }
                }
            });
            worker_thread.Start();

            return true;
        }

        void stop() {
            running.store(false);
            if (worker_thread.Sign()) {
                worker_thread.Stop();
            }
        }

        bool isRunning() const {
            return running.load();
        }

        // 获取当前系统支持的最小定时精度（纳秒）
        static long getMinResolution() {
            struct timespec res;
            if (clock_getres(CLOCK_MONOTONIC, &res) == 0) {
                return res.tv_nsec;
            }
            return 1000000; // 默认返回1ms
        }
    };
#endif
    class Timer {
        IntSleep m_sleep;
        std::atomic<bool> m_running{};
        std::thread m_thread;
        std::mutex m_mutex;
        int m_count = 0;
        static Map<int,std::function<void()>> m_callback;
#ifdef _WIN32
        HANDLE timerHandle = nullptr;
#else
        LinuxHighPrecisionTimer linux_timer;
#endif
    public:
        Timer();
        ~Timer();
        // 启动定时器
        void Start(const std::function<void()>& callback,unsigned long count,TimeType interval = TimeType::Millisecond);
        // 停止定时器
        void Stop();
    private:
        int AssignmentID();
#ifdef _WIN32
        static VOID CALLBACK TimerCallback(PVOID lpParam, BOOLEAN TimerOrWaitFired) {
            int count = *static_cast<int*>(lpParam);
            const auto func = m_callback.get(count);
            if (func) {
                try {
                    func->operator()();
                }
                catch (CCException& e) {
                    System::Println("Timer CallBack Error: {}",e.what());
                }
            }
        }
#else
#endif
    };
    class ClockCallBack{
        std::condition_variable cv;
        std::mutex mtx;
        size_t count = {};
        size_t sleep_time_ms = {};
        size_t timer_id = {};
        static CTL::Map<size_t,ClockCallBack*> m_callback;
        static CCMutex mutex;
        static std::atomic_bool is_running;
        static Thread* m_thread;
        static size_t getID(){
            for (int i = 0; i < 1000000; ++i) {
                if (!m_callback.IsContains(i)) {
                    return i;
                }
            }
            return -1;
        }
    public:
        static void Init(){
            is_running.store(true);
            m_thread = new Thread();
            m_thread->SetThread([&]() {
                while (is_running.load()) {
                    millisecond_delay(1);
                    CCUniqueLock lock(mutex);
                    for (const auto &item : m_callback) {
                        if (item.second && item.second->sleep_time_ms > 0) {
                            item.second->count++;
                            if (item.second->count >= item.second->sleep_time_ms) {
                                item.second->cv.notify_all();
                                item.second->count = 0;
                            }
                        }
                    }
                }
            });
            m_thread->Start();
        }
        static void Release(){
            is_running.store(false);
            delete m_thread;
        }
        void start(){
            CCUniqueLock lock(mutex);
            this->timer_id = getID();
            m_callback.put(timer_id,this);
        }
        void sleep(const size_t us){
            CCUniqueLock lock(mtx);
            this->sleep_time_ms = us;
            cv.wait_for(lock, std::chrono::milliseconds(sleep_time_ms));
        }
        void stop() const{
            CCUniqueLock lock(mutex);
            m_callback.Remove(timer_id);
        }
    };
}

#endif
