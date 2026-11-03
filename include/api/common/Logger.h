#pragma once
#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace api::common {

// 统一日志出口：线程安全，支持运行期调整级别
class Logger {
public:
    enum class Level { kDebug = 0, kInfo, kWarn, kError };

    static Logger& Instance() {
        static Logger logger;
        return logger;
    }

    void SetLevel(Level level) { minLevel_ = level; }

    void Log(Level level, const char* tag, const char* fmt, ...) {
        if (level < minLevel_) return;
        char buffer[2048];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        std::lock_guard<std::mutex> lock(mu_);
        std::fprintf(stderr, "[%s] [%s] %s\n", LevelName(level), tag, buffer);
    }

private:
    Logger() = default;

    static const char* LevelName(Level level) {
        switch (level) {
            case Level::kDebug: return "DEBUG";
            case Level::kInfo:  return "INFO";
            case Level::kWarn:  return "WARN";
            case Level::kError: return "ERROR";
        }
        return "?";
    }

    std::mutex mu_;
    Level minLevel_ = Level::kInfo;
};

} // namespace api::common

#define LOG_DEBUG(fmt, ...) ::api::common::Logger::Instance().Log(::api::common::Logger::Level::kDebug, __func__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  ::api::common::Logger::Instance().Log(::api::common::Logger::Level::kInfo,  __func__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  ::api::common::Logger::Instance().Log(::api::common::Logger::Level::kWarn,  __func__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) ::api::common::Logger::Instance().Log(::api::common::Logger::Level::kError, __func__, fmt, ##__VA_ARGS__)
