#include "CCTimer.h"
#include <CCThread.h>

size_t CTL::ClockTimer::initAccuracy = {};
CTL::Map<size_t,CTL::ClockCallBack*> CTL::ClockCallBack::m_callback;
CCMutex CTL::ClockCallBack::mutex;
std::atomic_bool CTL::ClockCallBack::is_running;
CTL::Thread* CTL::ClockCallBack::m_thread = nullptr;
CTL::Map<int,std::function<void()>> CTL::Timer::m_callback;

CTL::Timer::Timer() {
    m_running = false;
}

CTL::Timer::~Timer() {
    Stop();
}

void CTL::Timer::Start(const std::function<void()>& callback, const unsigned long count,
    const TimeType interval) {
    Stop(); // 确保之前的定时器已停止
    m_count = AssignmentID();
    const auto ms = [interval]() {
        switch (interval) {
        case TimeType::Millisecond:
            return static_cast<unsigned long>(1);
        case TimeType::Second:
            return static_cast<unsigned long>(1000);
        case TimeType::Minute:
            return static_cast<unsigned long>(1000 * 60);
        case TimeType::Hour:
            return static_cast<unsigned long>(1000 * 60 * 60);
        default:
            return static_cast<unsigned long>(1000);
        }
    };

#ifdef _WIN32
    unsigned long m_Time_ms = ms() * count;
    CreateTimerQueueTimer(
        &timerHandle,           // 定时器句柄
        nullptr,                   // 默认定时器队列
        TimerCallback,          // 回调函数
        &m_count,                   // 回调参数
        m_Time_ms,                   // 1秒后首次触发
        m_Time_ms,                   // 每隔1秒重复触发
        WT_EXECUTEDEFAULT       // 默认执行标志
    );
    m_callback.put(m_count, callback);
#elif __OHOS__
    m_running = true;
    m_thread = std::thread([=]() {
        while (m_running) {
            switch(interval) {
                case TimeType::Millisecond:
                    m_sleep.SleepMillisecond(std::chrono::milliseconds(count));
                    break;
                case TimeType::Second:
                    m_sleep.SleepSecond(std::chrono::seconds(count));
                    break;
                case TimeType::Minute:
                    m_sleep.SleepMinutes(std::chrono::minutes(count));
                    break;
                case TimeType::Hour:
                    m_sleep.SleepHour(std::chrono::hours(count));
                    break;
            }
            if (m_running) {
                callback();
            }
        }
    });
    m_thread.detach();
#elif defined(__linux__)
    // 使用 Linux 高精度定时器
    timespec interval_ts, initial_ts;

    // 将毫秒转换为 timespec 结构
    unsigned long total_ms = ms() * count;
    interval_ts.tv_sec = total_ms / 1000;
    interval_ts.tv_nsec = (total_ms % 1000) * 1000000;

    // 初始延迟设为相同的值
    initial_ts = interval_ts;

    // 启动 Linux 高精度定时器
    if (linux_timer.start([this]() {
        const auto func = m_callback.get(this->m_count);
        if (func) {
            try {
                func->operator()();
            }
            catch (CCException& e) {
                System::Println("Timer CallBack Error: {}",e.what());
            }
        }
    }, interval_ts, initial_ts)) {
        m_callback.put(m_count, callback);
    }
    else {
        System::Println("Failed to start Linux high precision timer");
    }
#else
    // 其他平台使用原有实现（需要完善）
    m_running = true;
    m_thread = std::thread([=]() {
        while (m_running) {
            switch(interval) {
                case TimeType::Millisecond:
                    m_sleep.SleepMillisecond(std::chrono::milliseconds(count));
                    break;
                case TimeType::Second:
                    m_sleep.SleepSecond(std::chrono::seconds(count));
                    break;
                case TimeType::Minute:
                    m_sleep.SleepMinutes(std::chrono::minutes(count));
                    break;
                case TimeType::Hour:
                    m_sleep.SleepHour(std::chrono::hours(count));
                    break;
            }
            if (m_running) {
                callback();
            }
        }
    });
    m_thread.detach();
#endif
}

void CTL::Timer::Stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
#ifdef _WIN32
        if (timerHandle) {
            DeleteTimerQueueTimer(nullptr, timerHandle, INVALID_HANDLE_VALUE);
            timerHandle = nullptr;
        }
#elif __OHOS__
        m_running = false;
        m_sleep.interrupt();
#elif defined(__linux__) || defined(__APPLE__)
        linux_timer.stop();
#else
        m_running = false;
        m_sleep.interrupt();
#endif
        m_callback.Remove(this->m_count);
        m_running = false;
    }
}

int CTL::Timer::AssignmentID(){
    CCUniqueLock lock(m_mutex);
    for (int i = 0; i < 100000; i++) {
        if (!m_callback.IsContains(i)) {
            return i;
        }
    }
    return -1;
}
