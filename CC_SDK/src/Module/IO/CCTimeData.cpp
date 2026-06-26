#include "CC.h"
#include "CCString.h"
using namespace CTL;

int daysInMonth_t[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

std::string CCTimeData::LastCheckedDate;

CCTimeData::CCTimeData(std::time_t* t) {
    m_time = t;
}

CCTimeData::CCTimeData(const CCTimeInfo &t) {
    GetTimeInfo = t;
}

CCTimeData::~CCTimeData() {
    if (m_time) {
        delete m_time;
    }
    if (p) {
        delete p;
    }
}

CCTimeInfo CCTimeData::Now() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    const std::tm* now_tm = std::localtime(&now_time_t);
    const int year = now_tm->tm_year + 1900;
    const int month = now_tm->tm_mon + 1;
    const int day = now_tm->tm_mday;
    const int hour = now_tm->tm_hour;
    const int minute = now_tm->tm_min;
    const int second = now_tm->tm_sec;
    const int weekday = GetWeek(now_tm->tm_wday);
    return CCTimeInfo{year, month, day, hour, minute, second, weekday};
}

CCTimeInfo CCTimeData::GetTime(const time_t t) {
    const std::tm* now_tm = std::localtime(&t);
    const int year = now_tm->tm_year + 1900;
    const int month = now_tm->tm_mon + 1;
    const int day = now_tm->tm_mday;
    const int hour = now_tm->tm_hour;
    const int minute = now_tm->tm_min;
    const int second = now_tm->tm_sec;
    const int weekday = GetWeek(now_tm->tm_wday);
    return CCTimeInfo{year, month, day, hour, minute, second, weekday};
}

int CCTimeData::GetWeek(const int week) {
    switch (week) {
        case 0: return 7;
        case 1: return 1;
        case 2: return 2;
        case 3: return 3;
        case 4: return 4;
        case 5: return 5;
        case 6: return 6;
        default: return -1;
    }
}

std::string CCTimeData::to_String(const StdString& format) const {
    // return String::format("%d-%d-%d %d:%d:%d", GetTimeInfo.year, GetTimeInfo.month,
    //                         GetTimeInfo.day,GetTimeInfo.hour,GetTimeInfo.minute,GetTimeInfo.second);
    String str = format;
    const int l1 = format.find("yyyy");
    const int l2 = format.find("MM");
    const int l3 = format.find("dd");
    const int l4 = format.find("HH");
    const int l7 = format.find("hh");
    const int l5 = format.find("mm");
    const int l6 = format.find("ss");
    if (l1 != String::npos && l1 != -1) {
        str.replace(l1, 4, String::format("%d",GetTimeInfo.year));
    }
    if (l2 != String::npos && l2 != -1) {
        str.replace(l2, 2, String::format("%02d",GetTimeInfo.month));
    }
    if (l3 != String::npos && l3 != -1) {
        str.replace(l3, 2, String::format("%02d",GetTimeInfo.day));
    }
    if (l4 != String::npos && l4 != -1) {
        str.replace(l4, 2, String::format("%02d",GetTimeInfo.hour));
    }
    else if (l7 != String::npos && l7 != -1) {
        str.replace(l7, 2, String::format("%02d",GetTimeInfo.hour - 12));
    }
    if (l5 != String::npos && l5 != -1) {
        str.replace(l5, 2, String::format("%02d",GetTimeInfo.minute));
    }
    if (l6 != String::npos && l6 != -1) {
        str.replace(l6, 2, String::format("%02d",GetTimeInfo.second));
    }
    return str;
}

std::string CCTimeData::Format(const std::string &format) const {
    // 创建一个足够大的字符数组来存储格式化后的时间字符串
    char buffer[80] = {0};
    // 使用 strftime 格式化时间
    std::strftime(buffer, sizeof(buffer), format.c_str(), p);
    // 将格式化后的时间字符串转换为 std::string 并返回
    return std::string(buffer);
}

CCTimeInfo CCTimeData::parseTimeString(const std::string &timeStr) {
    CCTimeInfo timeInfo = {};
    std::istringstream ss(timeStr);
    char delimiter;


    ss >> timeInfo.year >> delimiter >> timeInfo.month >> delimiter >> timeInfo.day
       >> timeInfo.hour >> delimiter >> timeInfo.minute >> delimiter >> timeInfo.second;

    // 注意：week 字段需要额外计算，这里暂时设为 0
    timeInfo.week = 0;

    return timeInfo;
}

time_t CCTimeData::parseTimeString_t(const std::string &timeStr) {
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    tm.tm_isdst = -1; // 让系统决定是否为夏令时
    return std::mktime(&tm);
}

std::string CCTimeData::formatTime(const time_t time) {
    const auto tm = localtime(&time);
    std::ostringstream ss;
    ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string CCTimeData::addSecondsToTime(const std::string &timeStr, int seconds) {
    time_t time = parseTimeString_t(timeStr);
    time += seconds; // 加上指定秒数
    return formatTime(time);
}

bool CCTimeData::operator==(const CCTimeInfo &other) const {
    return GetTimeInfo.year == other.year && GetTimeInfo.month == other.month &&
           GetTimeInfo.day == other.day && GetTimeInfo.hour == other.hour &&
           GetTimeInfo.minute == other.minute && GetTimeInfo.second == other.second;
}

bool CCTimeData::operator>(const CCTimeInfo &other) const {
    return GetTimeInfo.year > other.year ||
           (GetTimeInfo.year == other.year && GetTimeInfo.month > other.month) ||
           (GetTimeInfo.year == other.year && GetTimeInfo.month == other.month &&
            GetTimeInfo.day > other.day) ||
           (GetTimeInfo.year == other.year && GetTimeInfo.month == other.month &&
            GetTimeInfo.day == other.day && GetTimeInfo.hour > other.hour) ||
                (GetTimeInfo.year == other.year && GetTimeInfo.month == other.month &&
                 GetTimeInfo.day == other.day && GetTimeInfo.hour == other.hour &&
                 GetTimeInfo.minute > other.minute) ||
                (GetTimeInfo.year == other.year && GetTimeInfo.month == other.month &&
                 GetTimeInfo.day == other.day && GetTimeInfo.hour == other.hour &&
                 GetTimeInfo.minute == other.minute && GetTimeInfo.second >other.second);
}

bool CCTimeData::operator<(const CCTimeInfo &other) const {
    return GetTimeInfo.year < other.year ||
           (GetTimeInfo.year == other.year && GetTimeInfo.month < other.month) ||
           (GetTimeInfo.year == other.year && GetTimeInfo.month == other.month &&
            GetTimeInfo.day < other.day) ||
           (GetTimeInfo.year == other.year && GetTimeInfo.month == other.month &&
            GetTimeInfo.day == other.day && GetTimeInfo.hour < other.hour) ||
                (GetTimeInfo.year == other.year && GetTimeInfo.month == other.month &&
                 GetTimeInfo.day == other.day && GetTimeInfo.hour == other.hour &&
                 GetTimeInfo.minute < other.minute) ||
                (GetTimeInfo.year == other.year && GetTimeInfo.month == other.month &&
                 GetTimeInfo.day == other.day && GetTimeInfo.hour == other.hour &&
                 GetTimeInfo.minute == other.minute && GetTimeInfo.second <other.second);
}

int CCTimeData::operator-(const CCTimeInfo &other) const {
    return (GetTimeInfo.year - other.year) * 365 + (GetTimeInfo.month - other.month) * 30 +
           (GetTimeInfo.day - other.day) * 24 + (GetTimeInfo.hour - other.hour) * 60 +
           (GetTimeInfo.minute - other.minute) * 60 + (GetTimeInfo.second - other.second);
}

bool CCTimeData::isLeapYear(const int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int CCTimeData::dateToDays(const int year, const int month, const int day) {
    int days = 0;
    // 加上整年的天数
    for (int y = 1; y < year; ++y) {
        days += isLeapYear(y) ? 366 : 365;
    }
    // 加上当年前面月份的天数
    for (int m = 1; m < month; ++m) {
        days += daysInMonth_t[m - 1];
        if (m == 2 && isLeapYear(year)) {
            days += 1; // 闰年2月多一天
        }
    }
    // 加上当月的天数
    days += day;
    return days;
}

int CCTimeData::calculateDaysDifference(const std::string &date1, const std::string &date2) {
    const auto T1 = parseTimeString(date1);
    const auto T2 = parseTimeString(date2);
    const int days1 = dateToDays(T1.year, T1.month, T1.day);
    const int days2 = dateToDays(T2.year, T2.month, T2.day);
    return days2 - days1;
}

bool CCTimeData::IsNewDay() {
    const CCTimeData currentTime;
    const std::string currentDate = currentTime.to_String("yyyy-MM-dd");
    if (LastCheckedDate != currentDate) {
        LastCheckedDate = currentDate;
        return true;
    }
    return false;
}

CCTimeData::CCTimeData(const std::string &t) {
    this->GetTimeInfo = parseTimeString(t);
}
