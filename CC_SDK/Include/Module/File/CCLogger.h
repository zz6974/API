#ifndef CC_LOGGER_H
#define CC_LOGGER_H

#include "CC.h"
#include "CCTimer.h"
#include "CCJSONObject.h"
#include "TL/Map.h"

namespace CTL {
    /**
     * 日志等级枚举
     */
    enum Logger_Level {
        Debug = 0,
        Info,
        Warning,
        Error,
        Fatal
    };
    struct LogParentFile {
        JSON_TYPE_INTRUSIVE(LogParentFile,Name,Date);
        String Name = "";
        String Date = "";
        String File_Path = "";
    };
    struct LogFile_t {
        JSON_TYPE_INTRUSIVE(LogFile_t,Name,Date,Size);
        String Name;
        size_t Size = 0;
        String Date = "";
    };
    class Logger {
        String Root_Folder_Path = "./logs";
        String m_log_file_name;
        String m_log_file_dir;
        Logger_Level m_level = Logger_Level::Debug;
        int log_m_count = 0,log_s_count = 5,log_size_count = 1;
        CCQueue<String> m_log_s;
        const char * Level_Count[5] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};
        bool log_flag = false;
        int is_save_cond_ = 7;
        Timer m_log_thread;
        CCMutex _mutex_;
        Map<String,LogParentFile> m_log_map;
        String CurrentDate = "";
    public:
        Logger();
        ~Logger();
        /**
         * 设置日志存放的文件夹
         * @param Folder 文件夹路径
         */
        void SetLogFolder(const String& Folder);
        /**
         * 设置日志缓存数
         * @param count 最大日志缓存数
         */
        void SetLogCount(int count);
        /**
         * 设置一个日志文件大小
         * @param count 大小 单位KB
         */
        void SetLogSize(int count);
        /**
         * 设置日志保存天数
         * @param cond
         */
        void SetLogSave(int cond);
        /**
         * 设置日志等级
         * @param level 日志等级
         */
        void SetLevel(Logger_Level level);
        /**
         * 日志输出 Debug等级
         * @param fmt 日志内容
         */
        void Debug(const String& fmt);
        /**
         * 日志输出 Info等级
         * @param fmt 日志内容
         */
        void Info(const String& fmt);
        /**
         * 日志输出 Warning等级
         * @param fmt 日志内容
         */
        void Warning(const String& fmt);
        /**
         * 日志输出 Error等级
         * @param fmt 日志内容
         */
        void Error(const String& fmt);
        /**
         * 日志输出 Fatal等级
         * @param fmt 日志内容
         */
        void Fatal(const String& fmt);
        /**
         * 日志输出
         * @param level 日志等级
         * @param message 日志内容
         * @param file 源文件 __FILE__
         * @param line 行 __LINE__
         */
        void LogOutput(Logger_Level level, const String& message, const String& file= __FILE__, int line = __LINE__);
        /**
         * 开始日志线程 定时器 12 小时检查一次
         */
        void Start();
        /**
         * 停止日志线程
         */
        void Stop();
        /**
         * 判断是否闰年
         * @return 是否闰年
         */
        static bool isLeapYear(int year);
        /**
         * 计算从公元元年到指定日期的总天数
         * @return 总天数
         */
        int dateToDays(int year, int month, int day);
        /**
         * 解析日期字符串为年月日
         * @param dateStr 日期字符串 2025-03-01
         * @param year 年
         * @param month 月
         * @param day 日
         */
        void parseDateString(const std::string& dateStr, int& year, int& month, int& day);
        /**
         * 计算两个日期之间的天数差
         * @param date1 日期1
         * @param date2 日期2
         * @return 天数差
         */
        int calculateDaysDifference(const std::string& date1, const std::string& date2);
        /**
         * 获取日志列表
         * @return 日志列表
         */
        CCVector<LogParentFile> GetLogs();
        /**
         * 获取日志文件列表
         * @param Date 日志日期
         * @return 日志文件列表
         */
        CCVector<LogFile_t> GetLogFiles(const String& Date);
        /**
         * 获取当前日志文件夹
         * @return 日志文件夹
         */
        String GetCurrentFolder() const;
        /**
         * 获取当前日期
         * @return 当前日期
         */
        String GetCurrentDate() const;
        /**
         * 获取日志文件
         * @param Date 日志日期
         * @param fileName 日志文件名
         * @return 日志文件
         */
        String GetLogFile(const String& Date, const String& fileName);
        /**
         * 获取日志
         * @param Date 日志日期
         * @param fileName 日志文件名
         * @return 日志
         */
        String GetLog(const String& Date, const String& fileName);
        /**
         * 删除日志文件
         * @param Date 日志日期
         * @param fileName 日志文件名
         * @return 是否删除成功
         */
        bool DeleteLog(const String& Date, const String& fileName);
        /**
         * 将日志缓存写入文件
         */
        void Flush();
    private:
        void Log2(Logger_Level level, const String& message);
        bool LogWriting(const String& message);
        void WriteData(const String& Path);
        void LogThread();
        void InitializeFolder();
        void DeleteOldFolders(const String& rootPath, int days);
        String GetLogFilePath(const String& fileName);
        std::chrono::system_clock::time_point StringToDate(const String& dateString);
        bool CompareDateDifference(const String& dateStr1, const String& dateStr2, int days);
        String GetsTheCurrentLogFile();
        void CloseWrite();
        int daysBetweenDates(const std::string& date1, const std::string& date2);
    };
}


#define CC_Logger_Debug(LoggerObj,m,...) LoggerObj.LogOutput(CTL::Logger_Level::Debug,m, __FILE__, __LINE__)
#define CC_Logger_Info(LoggerObj,m,...) LoggerObj.LogOutput(CTL::Logger_Level::Info,m, __FILE__, __LINE__)
#define CC_Logger_Warning(LoggerObj,m,...) LoggerObj.LogOutput(CTL::Logger_Level::Warning,m, __FILE__, __LINE__)
#define CC_Logger_Error(LoggerObj,m,...) LoggerObj.LogOutput(CTL::Logger_Level::Error,m, __FILE__, __LINE__)
#define CC_Logger_Fatal(LoggerObj,m,...) LoggerObj.LogOutput(CTL::Logger_Level::Fatal,m, __FILE__, __LINE__)

#endif
