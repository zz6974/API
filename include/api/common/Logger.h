#pragma once
#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace api::common {

/**
 * @brief 统一日志出口
 *
 * 线程安全的日志单例，支持运行期调整级别，低于级别的日志被丢弃。
 */
class Logger {
public:
    /// 日志级别（数值越小越详细）
    enum class Level { kDebug = 0, kInfo, kWarn, kError };

    /**
     * @brief 单例入口（本模块唯一的全局状态，线程安全）
     * @return 全局唯一 Logger 实例
     */
    Logger(const Logger& logger) = delete;
    Logger& operator=(const Logger& logger) = delete;
    static Logger& Instance() {
        static Logger logger;
        return logger;
    }

    /**
     * @brief 运行期调整日志级别
     * @param level 新的最低输出级别
     */
    void SetLevel(Level level) { minLevel_ = level; }

    /**
     * @brief 线程安全的格式化输出
     * @param level 日志级别
     * @param tag 日志标签（通常传 __func__）
     * @param fmt printf 风格格式串
     * @param ... 可变参数
     */
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

    /**
     * @brief 级别转名称字符串
     * @param level 日志级别
     * @return 级别名称（DEBUG/INFO/WARN/ERROR）
     */
    static const char* LevelName(Level level) {
        switch (level) {
            case Level::kDebug: return "DEBUG";
            case Level::kInfo:  return "INFO";
            case Level::kWarn:  return "WARN";
            case Level::kError: return "ERROR";
        }
        return "?";
    }

    std::mutex mu_;                   ///< 输出互斥锁，防多线程串行错乱
    Level minLevel_ = Level::kInfo;   ///< 最低输出级别
};

} // namespace api::common

// 日志宏：自动把当前函数名（__func__）作为 tag
//两个核心作用：节省代码，提高可读性 /  自动添加调用的那个函数的函数名
#define LOG_DEBUG(fmt, ...) ::api::common::Logger::Instance().Log(::api::common::Logger::Level::kDebug, __func__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  ::api::common::Logger::Instance().Log(::api::common::Logger::Level::kInfo,  __func__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  ::api::common::Logger::Instance().Log(::api::common::Logger::Level::kWarn,  __func__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) ::api::common::Logger::Instance().Log(::api::common::Logger::Level::kError, __func__, fmt, ##__VA_ARGS__)
