#include "CCLogger.h"
#include <CCThread.h>
#include "CCFile.h"
#include "CCFIleInStream.h"
#include "CCSystem.h"

int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

CTL::Logger::Logger() {
    log_flag = true;

}

CTL::Logger::~Logger() {
    log_flag = false;
    Stop();
}

void CTL::Logger::SetLogFolder(const String& Folder) {
    Root_Folder_Path = Folder;
}

void CTL::Logger::SetLogCount(const int count) {
    log_s_count = count;
}

void CTL::Logger::SetLogSize(const int count) {
    log_size_count = count;
}

void CTL::Logger::SetLogSave(const int cond) {
    is_save_cond_ = cond;
}

void CTL::Logger::SetLevel(const Logger_Level level) {
    m_level = level;
}

void CTL::Logger::Debug(const String& fmt) {
    Log2(Logger_Level::Debug, fmt);
}

void CTL::Logger::Info(const String& fmt) {
    Log2(Logger_Level::Info, fmt);
}

void CTL::Logger::Warning(const String& fmt) {
    Log2(Logger_Level::Warning, fmt);
}

void CTL::Logger::Error(const String& fmt) {
    Log2(Logger_Level::Error, fmt);
}

void CTL::Logger::Fatal(const String& fmt) {
    Log2(Logger_Level::Fatal, fmt);
}

void CTL::Logger::LogOutput(const Logger_Level level, const String& message, const String& file, const int line) {
    if (m_level > level) {
        return;
    }
    const CCTimeData Time;
    const String T = Time.to_String();
    const String str = String::format("[%s %s %s:%d] -> %s",T.c_str() ,Level_Count[level]
        ,file.c_str(), line, message.c_str());
    if (level == Logger_Level::Debug) {
        CC::Println(str,BLUE);
    }
    else if (level == Logger_Level::Info) {
        CC::Println(str,GREEN);
    }
    else if (level == Logger_Level::Warning) {
        CC::Println(str,YELLOW);
    }
    else if (level == Logger_Level::Error) {
        CC::Println(str,RED);
    }
    else if (level == Logger_Level::Fatal) {
        CC::Println(str,RED);
    }
    LogWriting(str);
}

void CTL::Logger::Start() {
    m_log_file_name = GetsTheCurrentLogFile();
    InitializeFolder();
    const auto list = CCFile::DirectoryList(m_log_file_dir);
    for (auto& I : list) {
        CCFile file(I);
        if (const bool F = file.GetFileSize() > log_size_count * 1024) {
            log_m_count++;
        }
        else {
            break;
        }
    }
    auto func = [this] { LogThread(); };
    m_log_thread.Start(func, 12,TimeType::Hour);
}

void CTL::Logger::Stop() {
    m_log_thread.Stop();
    CloseWrite();
}

bool CTL::Logger::isLeapYear(const int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int CTL::Logger::dateToDays(int year, int month, int day) {
    int days = 0;
    // 加上整年的天数
    for (int y = 1; y < year; ++y) {
        days += isLeapYear(y) ? 366 : 365;
    }
    // 加上当年前面月份的天数
    for (int m = 1; m < month; ++m) {
        days += daysInMonth[m - 1];
        if (m == 2 && isLeapYear(year)) {
            days += 1; // 闰年2月多一天
        }
    }
    // 加上当月的天数
    days += day;
    return days;
}

void CTL::Logger::parseDateString(const std::string &dateStr, int &year, int &month, int &day) {
    sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day);
}

int CTL::Logger::calculateDaysDifference(const std::string &date1, const std::string &date2) {
    int year1, month1, day1;
    int year2, month2, day2;

    parseDateString(date1, year1, month1, day1);
    parseDateString(date2, year2, month2, day2);

    int days1 = dateToDays(year1, month1, day1);
    int days2 = dateToDays(year2, month2, day2);

    return days2 - days1;
}

CCVector<CTL::LogParentFile> CTL::Logger::GetLogs() {
    return m_log_map.values();
}

CCVector<CTL::LogFile_t> CTL::Logger::GetLogFiles(const String &Date) {
    CCVector<LogFile_t> result;
    const auto Item = m_log_map.get(Date);
    if (Item) {
        String Path = "";
#ifdef _WIN32
        Path = CCFile::NormalizePath("/" + Item->File_Path);
#else
        Path = CCFile::NormalizePath("/" + Item->File_Path);
#endif
        // CTL::System::Println("Path: {}",Path.c_str());
        // CTL::System::Println("File_Path: {}",Item->File_Path.c_str());
        const auto FileList = CCFile::GetDirectoryContents(Path);
        for (const auto& File : FileList) {
            try {
                LogFile_t file;
                file.Name = File.Name;
                file.Size = File.Size;
                file.Date = CCTimeData::formatTime(File.LastWriteTime);
                result.add_lock(file);
            }
            catch (...){}
        }
    }
    return result;
}

CTL::String CTL::Logger::GetCurrentFolder() const {
    const String RP = CCFile::NormalizePath(Root_Folder_Path);
    const String RPA = CCFile::NormalizePath(RP + "/" + "log_" + CurrentDate);
    return RPA;
}

CTL::String CTL::Logger::GetCurrentDate() const {
    return CurrentDate;
}

CTL::String CTL::Logger::GetLogFile(const String &Date, const String &fileName) {
    const auto List = GetLogFiles(Date);
    for (const auto &I : List) {
        if (I.Name == fileName) {
            try {
                const String RP = CCFile::NormalizePath(Root_Folder_Path);
                const String RPA = CCFile::NormalizePath(RP + "/" + "log_" + Date);
                return RPA + "/" + I.Name;
            }
            catch (...){}
            break;
        }
    }
    return "";
}

CTL::String CTL::Logger::GetLog(const String &Date, const String &fileName) {
    const auto List = GetLogFiles(Date);
    for (const auto &I : List) {
        if (I.Name == fileName) {
            try {
                const String RP = CCFile::NormalizePath(Root_Folder_Path);
                const String RPA = CCFile::NormalizePath(RP + "/" + "log_" + Date);
                const String Path = RPA + "/" + I.Name;
                String str = CCFileInStream::ReadFileDataAll(Path);
                return str;
            }
            catch (...){}
        }
    }
    return "";
}

bool CTL::Logger::DeleteLog(const String &Date, const String &fileName) {
    bool F = false;
    const auto List = GetLogFiles(Date);
    for (const auto &I : List) {
        if (I.Name == fileName) {
            try {
                const String RP = CCFile::NormalizePath(Root_Folder_Path);
                const String RPA = CCFile::NormalizePath(RP + "/" + "log_" + Date);
                const String Path = RPA + "/" + I.Name;
                CCFile file(Path);
                if (file.isExists()) {
                    F = file.Delete(false);
                }
            }
            catch (...){}
            break;
        }
    }
    return F;
}

void CTL::Logger::Flush() {
    m_log_file_name = GetsTheCurrentLogFile();
    WriteData(m_log_file_name);
}

void CTL::Logger::Log2(const Logger_Level level, const String& message) {
    if (m_level > level) {
        return;
    }
    const CCTimeData Time;
    const String T = Time.to_String();
    const String str = String::format("[%s] %s -> %s",T.c_str() ,Level_Count[level],message.c_str());
    const auto OS = System::GetOS();
    if (level == Logger_Level::Debug) {
        if (OS == OS::OpenHarmony) {
            System::Println(str);
        }
        else {
            CC::Println(str,BLUE);
        }
    }
    else if (level == Logger_Level::Info) {
        if (OS == OS::OpenHarmony) {
            System::Println(str);
        }
        else {
            CC::Println(str,GREEN);
        }
    }
    else if (level == Logger_Level::Warning) {
        if (OS == OS::OpenHarmony) {
            System::Println(str);
        }
        else {
            CC::Println(str,YELLOW);
        }
    }
    else if (level == Logger_Level::Error || Logger_Level::Fatal) {
        if (OS == OS::OpenHarmony) {
            System::Println(str);
        }
        else {
            CC::Println(str,RED);
        }
    }
    LogWriting(str);
}

bool CTL::Logger::LogWriting(const String& message) {
    _mutex_.lock();
    m_log_s.add_lock(message + "\n");
    if (m_log_s.size() < log_s_count) {
        _mutex_.unlock();
        return false;
    }
    m_log_file_name = GetsTheCurrentLogFile();
    WriteData(m_log_file_name);
    _mutex_.unlock();
    return true;
}

void CTL::Logger::WriteData(const String& Path) {
    std::ofstream file(Path, std::ios::app);
    if (file.is_open()) {
        while (true) {
            if (!m_log_s.IsEmpty()) {
                const String str = m_log_s.poll_lock();
                file << str;
            }
            else{
                file.close();
                break;
            }
        }
    }
    const CCFile file_t(m_log_file_name);
    if (const bool F = file_t.GetFileSize() > log_size_count * 1024) {
        log_m_count++;
    }
}

void CTL::Logger::LogThread() {
    InitializeFolder();
}

void CTL::Logger::InitializeFolder() {
    const CCFile f(Root_Folder_Path);
    if (!f.isExists()) {
        CCFile::Create(Root_Folder_Path, true);
    }
    else {
        const auto list = CCFile::DirectoryList(Root_Folder_Path);
        const CCTimeData Time;
        const String baseDate = Time.to_String("yyyy-MM-dd");
        for (const auto& entry : list) {
            if (entry.is_directory()) {
                CCFile file(entry);
                const String Time2 = GetLogFilePath(file.GetName());
                if (!Time2.empty()) {
                    if (CompareDateDifference(baseDate, Time2,is_save_cond_)) {
                        bool F = file.Delete(true);
                        if (F) {
                            m_log_map.Remove(Time2);
                        }
                    }
                    else {
                        LogParentFile logger_t;
                        logger_t.Date = Time2;
                        logger_t.Name = file.GetName();
                        logger_t.File_Path = file.GetPath();
                        m_log_map.put(logger_t.Date, logger_t);
                    }
                }
            }
        }
    }
}

void CTL::Logger::DeleteOldFolders(const String& rootPath, const int days) {
    const auto list = CCFile::DirectoryList(rootPath);
    const auto now = std::chrono::system_clock::now();
    const auto duration = std::chrono::hours(24 * days);
    const auto cutoff_time = now - duration;
    for (const auto& entry : list) {
        if (entry.is_directory()) {
            const auto last_write_time = CC_FS::last_write_time(entry.path());
            const auto last_write_time_sys =
                std::chrono::system_clock::from_time_t(std::chrono::duration_cast<std::chrono::seconds>(
                    last_write_time.time_since_epoch()).count());
            if (last_write_time_sys < cutoff_time) {
                try {
                    CC_FS::remove_all(entry.path());
                }
                catch (const CC_FS::filesystem_error& e) {

                }
            }
        }
    }
}

CTL::String CTL::Logger::GetLogFilePath(const String& fileName) {
    // 找到日期的起始位置
    const size_t startPos = fileName.find_first_of("0123456789");
    if (startPos == std::string::npos) {
        return ""; // 如果没有找到数字，返回空字符串
    }
    // 提取日期部分（假设日期格式为 yyyy-mm-dd）
    return fileName.substr(startPos, 10);
}

std::chrono::system_clock::time_point CTL::Logger::StringToDate(const String& dateString) {
#ifdef _WIN32
    std::tm tm = {};
    std::istringstream ss(dateString.c_str());
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (ss.fail()) {
        throw std::runtime_error("Invalid date format");
    }
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
#else
    std::tm tm = {};
    const auto result = strptime(dateString.c_str(), "%Y-%m-%d", &tm);
    if (result == nullptr) {
        throw std::runtime_error("Invalid date format");
    }
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
#endif
}

bool CTL::Logger::CompareDateDifference(const String& dateStr1, const String& dateStr2, int days) {
    // auto date1 = StringToDate(dateStr1);
    // auto date2 = StringToDate(dateStr2);
    // auto duration = std::chrono::duration_cast<std::chrono::hours>(date2 - date1);
    const int daysDiff = daysBetweenDates(dateStr1, dateStr2);
    return daysDiff >= days || daysDiff <= -days;
}

CTL::String CTL::Logger::GetsTheCurrentLogFile() {
    const CCTimeData Time;
    CurrentDate = Time.to_String("yyyy-MM-dd");
    const String RP = CCFile::NormalizePath(Root_Folder_Path);
    m_log_file_dir = RP + "/" + "log_" + CurrentDate;
    const String RPA = CCFile::NormalizePath(m_log_file_dir);
    const CCFile f(RPA);
    if (!f.isExists()) {
        log_m_count = 0;
        CCFile::Create(RPA, true);
        LogParentFile logger_t;
        logger_t.Date = GetLogFilePath(f.GetName());
        logger_t.Name = f.GetName();
        logger_t.File_Path = f.GetPath();
        m_log_map.put(logger_t.Date, logger_t);
    }
    const String Path = RPA + "/" + CurrentDate + "_CC_" + CCInt(log_m_count).to_String() + ".log";
    m_log_file_name = CCFile::NormalizePath(Path);
    return m_log_file_name;
}

void CTL::Logger::CloseWrite() {
    m_log_file_name = GetsTheCurrentLogFile();
    WriteData(m_log_file_name);
}

int CTL::Logger::daysBetweenDates(const std::string &date1, const std::string &date2) {
    auto tp1 = StringToDate(date1);
    auto tp2 = StringToDate(date2);

    auto duration = std::chrono::duration_cast<std::chrono::hours>(tp2 - tp1);
    return duration.count() / 24; // 转换为天数
}
