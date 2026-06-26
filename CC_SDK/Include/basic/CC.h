#ifndef CC_H
#define CC_H
#pragma once

#if defined(__arm__) || defined(__aarch64__)
    #define IS_ARM 1
    #if defined(__aarch64__)
        #define IS_ARM64 1
    #else
        #define IS_ARM32 1
    #endif
#else
    #define IS_ARM 0
#endif

#if defined(__APPLE__) || defined(__linux__) || defined(__OHOS__) || defined(__ANDROID__)
#define CC_SYS_UNIX_POSIX 1
#elif defined(_WIN32)
#define CC_SYS_WIN_POSIX 1
#endif

#define CCTime_Day_S 86400
#define CCTime_Points_S 60

#include "CCMap.h"
#include "CCList.h"
#include "CCQueue.h"
#include "CCArray.h"
#include "CCVector.h"
#include "CCFunction.h"
#include "CCArrayList.h"
#include "CCLinkedMap.h"
#include <cstdint>
#include <fstream>
#include "mutex"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <utility>
#include "functional"
#include <chrono>
#include <shared_mutex>
#include "CCAutoLock.h"
#include <stdexcept>
#include <future>
#include <chrono>
#include <regex>
#include <csetjmp>
#include <signal.h>
#include "basic/basic.h"

#include "CCString.h"
#include "StreamHandler.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <Windows.h>
#pragma warning(disable : 4996)
#pragma comment(lib,"ws2_32.lib")
#elif __linux__
#include <unistd.h>
#include <csignal>
#endif


#if __OHOS__
#pragma warning(push)
#pragma warning(disable: -Wformat-security)
// 或者对于 GCC/Clang
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

#define CCVar auto
#define C_Auto const auto
using StdString = std::string;
typedef std::exception CCException;
typedef std::ostream OStream;

// 定义颜色代码
#define RESET   "\033[0m"
#define BLACK   "\033[30m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

// 定义背景色代码
#define BG_BLACK   "\033[40m"
#define BG_RED     "\033[41m"
#define BG_GREEN   "\033[42m"
#define BG_YELLOW  "\033[43m"
#define BG_BLUE    "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN    "\033[46m"
#define BG_WHITE   "\033[47m"

// 定义格式属性代码
#define BOLD      "\033[1m"
#define UNDERLINE "\033[4m"
#define BLINK     "\033[5m"
#define REVERSE   "\033[7m"

namespace CTL{
    template <typename T>
    OStream& operator<<(OStream& os, const ArrayList<T>& arr) {
        os << "[";
        for (size_t i = 0; i < arr.size(); ++i) {
            if (i != 0) {
                os << ", ";
            }
            os << arr[i];
        }
        os << "]";
        return os;
    }
    inline std::string generateUnique16CharString() {
        // 获取高精度时间戳（纳秒）
        auto now = std::chrono::high_resolution_clock::now();
        auto nanos = std::chrono::time_point_cast<std::chrono::nanoseconds>(now)
                         .time_since_epoch()
                         .count();

        // 转换为16位十六进制字符串（大写）
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(16)
           << static_cast<uint64_t>(nanos);
        return ss.str();
    }
}

// 结构体CCTimeInfo用于存储时间信息
struct CCTimeInfo
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int week;
};

// 类CCTimeData用于处理时间相关的操作
class CCTimeData
{
protected:
    time_t * m_time{};
    struct tm * p{};
public:
    // 默认构造函数，获取当前时间信息
    CCTimeData() {
        GetTimeInfo = Now();
    }
    // 构造函数，传入特定的时间点
    CCTimeData(std::time_t* t);
    CCTimeData(const CCTimeInfo &t);
    CCTimeData(const std::string &t);
    ~CCTimeData();
    // 获取当前时间信息
    static CCTimeInfo Now();
    static CCTimeInfo GetTime(time_t t);
    // 将时间信息转换为字符串
    /*
     * format 格式化字符串
     * 返回格式化后的字符串
     *
     * yyyy 年份
     * MM 月份
     * dd 十进制表示的每月的第几天
     * HH 十时制表示的小时
     * hh 12小时制的小时
     * mm 分钟数
     * ss 秒数
     */
    [[nodiscard]] std::string to_String(const StdString& format = "yyyy-MM-dd HH:mm:ss") const;
    // 根据指定格式格式化时间信息
    /*
    %a 星期几的简写
    %A 星期几的全称
    %b 月分的简写
    %B 月份的全称
    %c 标准的日期的时间串
    %C 年份的后两位数字
    %d 十进制表示的每月的第几天
    %D 月/天/年
    %e 在两字符域中，十进制表示的每月的第几天
    %F 年-月-日
    %g 年份的后两位数字，使用基于周的年
    %G 年分，使用基于周的年
    %h 简写的月份名
    %H 24小时制的小时
    %I 12小时制的小时
    %j 十进制表示的每年的第几天
    %m 十进制表示的月份
    %M 十时制表示的分钟数
    %n 新行符
    %p 本地的AM或PM的等价显示
    %r 12小时的时间
    %R 显示小时和分钟：hh:mm
    %S 十进制的秒数
    %t 水平制表符
    %T 显示时分秒：hh:mm:ss
    %u 每周的第几天，星期一为第一天 （值从0到6，星期一为0）
    %U 第年的第几周，把星期日做为第一天（值从0到53）
    %V 每年的第几周，使用基于周的年
    %w 十进制表示的星期几（值从0到6，星期天为0）
    %W 每年的第几周，把星期一做为第一天（值从0到53）
    %x 标准的日期串
    %X 标准的时间串
    %y 不带世纪的十进制年份（值从0到99）
    %Y 带世纪部分的十制年份
    %z，%Z 时区名称，如果不能得到时区名称则返回空字符。
    %% 百分号
     */
    [[nodiscard]] std::string Format(const std::string& format = "%Y-%M-%d %H:%M:%S") const;
    // 解析格式 "yyyy-MM-dd HH:mm:ss
    static CCTimeInfo parseTimeString(const std::string& timeStr);
    // 将时间字符串解析为 time_t
    static time_t parseTimeString_t(const std::string& timeStr);
    // 将 time_t 转换为格式化字符串
    static std::string formatTime(time_t time);
    // 计算时间加上秒数后的结果
    static std::string addSecondsToTime(const std::string& timeStr, int seconds);
    // 检查当前时间是否是新的一天
    static bool IsNewDay();
    // 比较时间信息
    bool operator==(const CCTimeInfo& other) const;
    // 比较时间是否晚于指定的时间信息
    bool operator>(const CCTimeInfo& other) const;
    // 比较时间是否早于指定的时间信息
    bool operator<(const CCTimeInfo& other) const;
    // 比叫时间与指定时间的差值
    int operator-(const CCTimeInfo& other) const;
    /**
     * 判断是否闰年
     * @return 是否闰年
    */
    static bool isLeapYear(int year);
    /**
     * 计算从公元元年到指定日期的总天数
     * @return 总天数
     */
    static int dateToDays(int year, int month, int day);
    /**
     * 计算两个日期之间的天数差
     * @param date1 日期1
     * @param date2 日期2
     * @return 天数差
     */
    static int calculateDaysDifference(const std::string& date1, const std::string& date2);
private:
    // 静态函数，用于获取星期几
    static int GetWeek(int week);
    // 存储时间信息的变量
    CCTimeInfo GetTimeInfo{};
    static std::string LastCheckedDate;
public:

};

// 类CC是一个工具类，包含了一些静态方法和属性
class CC{
    static std::mutex M_mutex_Print;
public:
    typedef int RIos;
public:
    template<typename... Args>
    static StdString format(const char* fmt, Args... args) {
        std::string fmt_str = fmt;
        if (fmt_str.find("{}") != std::string::npos) {
            return CTL::format_braces_template(fmt_str, args...);
        }
        const int length = std::snprintf(nullptr, 0, fmt, args...);
        if (length <= 0) {
            return fmt_str; // 格式化失败，返回原始格式字符串
        }

        // 分配足够的空间来存储格式化后的字符串
        std::string result(length, '\0');
        std::snprintf(&result[0], length + 1, fmt, args...);
        return result;
    }
    // 设置程序关闭时的回调函数
    inline static void SetCloseFun(const std::function<bool()>& obj) {
        CloseFun = obj;
    }
    // 将整数转换为字符串
    static StdString to_String(const int v){
        return std::to_string(v);
    }
    // 将双精度浮点数转换为字符串
    static StdString to_String(const double v){
        return std::to_string(v);
    }
    // 将布尔值转换为字符串
    static StdString to_String(const bool v){
        return v ? "true" : "false";
    }
    // 将C风格字符串转换为StdString
    static StdString to_String(const char* v){
        return v;
    }
    // 直接返回传入的StdString
    static StdString to_String(StdString v){
        return v;
    }
    // 将指针转换为字符串
    static StdString to_String(void* v){
        return std::to_string((size_t)v);
    }
    static StdString to_String(const CTL::Byte v){
        return std::to_string(v.get());
    }
    static StdString to_String(const long v){
        return std::to_string(v);
    }
    static StdString to_String(const long unsigned int& v){
        return std::to_string(v);
    }
    static StdString to_String(const long long v){
        return std::to_string(v);
    }
    static StdString to_String(const long double v){
        return std::to_string(v);
    }
    // 打印输出，不换行
    template<typename T>
    static void Print(const T& v,const char* Color = RESET){
        customCout << Color << v << RESET;
    }
    // 格式化打印输出，不换行
    template<typename T,typename... Args>
    static void Print(long long size,const T& v,Args&&... args){
        CCVar* va = new char[size];
        memset(va,0,size);
        sprintf(va,v,args...);
        Print(va);
        delete[] va;
    }
    // 格式化打印输出，并换行
    template<typename T>
    static void Println(const T& v,const char* Color = RESET){
        std::unique_lock<std::mutex> lock_t(M_mutex_Print);
        customCout<< Color << v << RESET << std::endl;
    }
    // 带时间戳的打印输出，并换行
    template<typename T>
    static void TimePrintln(const T& v){
        StdString time = "[" + CCTimeData().to_String() + "] -> ";
        time = time + v;
        Println(time);
    }
    // 带时间戳的格式化打印输出，并换行
    template<typename T,typename... Args>
    static void TimePrintln(long long size,const T& v,Args&&... args){
        CCVar* va = new char[size];
        memset(va,0,size);
        sprintf(va,v,args...);
        TimePrintln(va);
        delete[] va;
    }
    // 打印错误信息，并抛出异常
    static void ErrorPrintln(const StdString& v){
        throw std::out_of_range(v.c_str());
    }
    // CCLogo打印相关的属性和方法
    static int LogoTimeMS;
    static void CC_Print_Logo(){
        // 打印CCLogo，每个部分之间暂停一段时间
        customCout << "        CCCCCCCCCCCCC        CCCCCCCCCCCCC                                       AAA               PPPPPPPPPPPPPPPPP   IIIIIIIIII  " << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "     CCC::::::::::::C     CCC::::::::::::C                                      A:::A              P::::::::::::::::P  I::::::::I" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "   CC:::::::::::::::C   CC:::::::::::::::C                                     A:::::A             P::::::PPPPPP:::::P I::::::::I" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "  C:::::CCCCCCCC::::C  C:::::CCCCCCCC::::C                                    A:::::::A            PP:::::P     P:::::PII::::::II" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << " C:::::C       CCCCCC C:::::C       CCCCCC                                   A:::::::::A             P::::P     P:::::P  I::::I  " << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "C:::::C              C:::::C                                                A:::::A:::::A            P::::P     P:::::P  I::::I  " << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "C:::::C              C:::::C                                               A:::::A A:::::A           P::::PPPPPP:::::P   I::::I  " << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "C:::::C              C:::::C                                              A:::::A   A:::::A          P:::::::::::::PP    I::::I  " << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "C:::::C              C:::::C                                             A:::::A     A:::::A         P::::PPPPPPPPP      I::::I  " << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "C:::::C              C:::::C                                            A:::::AAAAAAAAA:::::A        P::::P              I::::I  " << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "C:::::C              C:::::C                                           A:::::::::::::::::::::A       P::::P              I::::I  " << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << " C:::::C       CCCCCC C:::::C       CCCCCC                            A:::::AAAAAAAAAAAAA:::::A      P::::P              I::::I  " << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "  C:::::CCCCCCCC::::C  C:::::CCCCCCCC::::C                           A:::::A             A:::::A   PP::::::PP          II::::::II" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "   CC:::::::::::::::C   CC:::::::::::::::C                          A:::::A               A:::::A  P::::::::P          I::::::::I" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "     CCC::::::::::::C     CCC::::::::::::C                         A:::::A                 A:::::A P::::::::P          I::::::::I" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "        CCCCCCCCCCCCC        CCCCCCCCCCCCC                        AAAAAAA                   AAAAAAAPPPPPPPPPP          IIIIIIIIII" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "                                          ________________________                                                                "<< std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(LogoTimeMS));
        customCout << "                                          _:::::::::::::::::::::_                                                                " << std::endl;
        customCout << "                                          ________________________                                                                "<< std::endl;
        customCout << "                                                                                                                                 " << std::endl;
    }
    // 获取程序关闭时的回调函数
    static std::function<bool()> GetCloseFun() {
        return CloseFun;
    }
    static bool IsRun(){
        return Exe_Run;
    }
    static void Exit(){
        Exe_Run = false;
    }
    static std::string toLowerCase(const std::string& str) {
        std::string str_t = str;
        for (char& c : str_t) {
            c = std::tolower(c);
        }
        return str_t;
    }
private:
    // 程序关闭时的回调函数
    static std::function<bool()> CloseFun;
    static bool Exe_Run;
};



namespace CTL {
    inline void SignalHandler(const int signum) {
        signal(SIGINT, CTL::SignalHandler);   // Ctrl+C
        signal(SIGTERM, CTL::SignalHandler);  // 终止请求
        if (signum == 2) {
            if(const auto CFun = CC::GetCloseFun()){
                if (CFun()) {
                    CC::Exit();
                }
            }
            else {
                CC::Exit();
            }
        }
    }
}

class CCAppInit_ {
public:
    static std::function<void(int)> Signal;
    CCAppInit_() {
        signal(SIGINT, CTL::SignalHandler);   // Ctrl+C
        signal(SIGTERM, CTL::SignalHandler);  // 终止请求
#if __linux__
#endif
#ifndef CC_LOGO_NO
        CC::CC_Print_Logo();
#endif
    }
    ~CCAppInit_() {
        CC::Println("CC API library END OF RUN ! \u263A \u2764");
    }
};

// CCInt类用于封装整型数值
class CCInt
{
private:
    int value; // 存储整型数值的变量

public:
    // 默认构造函数
    CCInt() = default;

    // 构造函数，初始化整型数值
    // 参数v: 要封装的整型数值
    CCInt(const int v){
        this->value = v;
    }

    // 将整型数值转换为字符串表示
    // 返回值: 整型数值的字符串表示
    [[nodiscard]] StdString to_String() const{
        return std::to_string(value);
    }

    // 显式转换为整型数值
    // 返回值: 封装的整型数值
    operator int() const{
        return value;
    }
    operator StdString () const{
        return std::to_string(value);
    }
    operator StdString (){
        return std::to_string(value);
    }
    bool operator == (const CCInt& other) const{
        return value == other.value;
    }
    bool operator == (const int& other) const{
        return value == other;
    }
    bool operator != (const CCInt& other) const{
        return value != other.value;
    }
    bool operator != (const int& other) const{
        return value != other;
    }
    bool operator > (const CCInt& other) const{
        return value > other.value;
    }
    bool operator > (const int& other) const{
        return value > other;
    }
    bool operator < (const CCInt& other) const{
        return value < other.value;
    }
    bool operator < (const int& other) const{
        return value < other;
    }
    bool operator >= (const CCInt& other) const{
        return value >= other.value;
    }
    bool operator >= (const int& other) const{
        return value >= other;
    }
    bool operator <= (const CCInt& other) const{
        return value <= other.value;
    }
    bool operator <= (const int& other) const{
        return value <= other;
    }
    int operator + (const CCInt& other) const{
        return value + other.value;
    }
    int operator + (const int& other) const{
        return value + other;
    }
    int operator - (const CCInt& other) const{
        return value - other.value;
    }
    int operator - (const int& other) const{
        return value - other;
    }
    int operator * (const CCInt& other) const{
        return value * other.value;
    }
    int operator * (const int& other) const{
        return value * other;
    }
    int operator / (const CCInt& other) const{
        return value / other.value;
    }
    int operator / (const int& other) const{
        return value / other;
    }
    int operator % (const CCInt& other) const{
        return value % other.value;
    }
    int operator % (const int& other) const{
        return value % other;
    }
    int operator ++ (){
        return ++value;
    }
    int operator -- (){
        return --value;
    }
    int operator & (const CCInt& other) const{
        return value & other.value;
    }
    int operator & (const int& other) const{
        return value & other;
    }
    int operator | (const CCInt& other) const{
        return value | other.value;
    }
    int operator | (const int& other) const{
        return value | other;
    }
    int operator ^ (const CCInt& other) const{
        return value ^ other.value;
    }
    int operator ^ (const int& other) const{
        return value ^ other;
    }
    int operator << (const CCInt& other) const{
        return value << other.value;
    }
    int operator << (const int& other) const{
        return value << other;
    }
    int operator >> (const CCInt& other) const{
        return value >> other.value;
    }
    int operator >> (const int& other) const{
        return value >> other;
    }
    int operator && (const CCInt& other) const{
        return value && other.value;
    }
    int operator && (const int& other) const{
        return value && other;
    }
    int operator || (const CCInt& other) const{
        return value || other.value;
    }
    int operator || (const int& other) const{
        return value || other;
    }
    int operator ! (){
        return !value;
    }
    int operator ~ (){
        return ~value;
    }
    int operator = (const CCInt& other){
        value = other.value;
        return value;
    }
    int operator = (const int& other){
        value = other;
        return value;
    }
    int operator -= (const CCInt& other){
        value -= other.value;
        return value;
    }
    int operator -= (const int& other){
        value -= other;
        return value;
    }
    int operator += (const CCInt& other){
        value += other.value;
        return value;
    }
    int operator += (const int& other){
        value += other;
        return value;
    }
    int operator *= (const CCInt& other){
        value *= other.value;
        return value;
    }
    int operator *= (const int& other){
        value *= other;
        return value;
    }
    int operator /= (const CCInt& other){
        value /= other.value;
        return value;
    }
    int operator /= (const int& other){
        value /= other;
        return value;
    }
};

// CCDouble类用于封装双精度浮点数值
class CCDouble
{
private:
    double value; // 存储双精度浮点数值的变量

public:
    // 默认构造函数
    CCDouble() = default;

    // 构造函数，初始化双精度浮点数值
    // 参数v: 要封装的双精度浮点数值
    CCDouble(const double v){
        this->value = v;
    }

    // 将双精度浮点数值转换为字符串表示
    // 参数x: 小数点后的位数，默认为5
    // 返回值: 双精度浮点数值的字符串表示
    [[nodiscard]] StdString to_String(const int x = 5) const{
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(x) << value;
        return stream.str();
    }
    // 显式转换为双精度浮点数值
    operator double() const{
        return value;
    }
    operator StdString () const{
        return std::to_string(value);
    }
    bool operator == (const CCDouble& other) const{
        return value == other.value;
    }
    bool operator == (const double& other) const{
        return value == other;
    }
    bool operator != (const CCDouble& other) const{
        return value != other.value;
    }
    bool operator != (const double& other) const{
        return value != other;
    }
    bool operator > (const CCDouble& other) const{
        return value > other.value;
    }
    bool operator > (const double& other) const{
        return value > other;
    }
    bool operator < (const CCDouble& other) const{
        return value < other.value;
    }
    bool operator < (const double& other) const{
        return value < other;
    }
    bool operator >= (const CCDouble& other) const{
        return value >= other.value;
    }
    bool operator >= (const double& other) const{
        return value >= other;
    }
    bool operator <= (const CCDouble& other) const{
        return value <= other.value;
    }
    bool operator <= (const double& other) const{
        return value <= other;
    }
    double operator + (const CCDouble& other) const{
        return value + other.value;
    }
    double operator + (const double& other) const{
        return value + other;
    }
    double operator - (const CCDouble& other) const{
        return value - other.value;
    }
    double operator - (const double& other) const{
        return value - other;
    }
    double operator * (const CCDouble& other) const{
        return value * other.value;
    }
    double operator * (const double& other) const{
        return value * other;
    }
    double operator / (const CCDouble& other) const{
        return value / other.value;
    }
    double operator / (const double& other) const{
        return value / other;
    }
    double operator ++ (){
        return ++value;
    }
    double operator -- (){
        return --value;
    }
    double operator = (const CCDouble& other){
        value = other.value;
        return value;
    }
    double operator = (const double& other){
        value = other;
        return value;
    }
    double operator -= (const CCDouble& other){
        value -= other.value;
        return value;
    }
    double operator -= (const double& other){
        value -= other;
        return value;
    }
    double operator += (const CCDouble& other){
        value += other.value;
        return value;
    }
    double operator += (const double& other){
        value += other;
        return value;
    }
    double operator *= (const CCDouble& other){
        value *= other.value;
        return value;
    }
    double operator *= (const double& other) {
        value *= other;
        return value;
    }
    double operator /= (const CCDouble& other){
        value /= other.value;
        return value;
    }
    double operator /= (const double& other){
        value /= other;
        return value;
    }
};

// CCFloat类用于封装单精度浮点数值
class CCFloat
{
private:
    float value; // 存储单精度浮点数值的变量

public:
    // 默认构造函数
    CCFloat() = default;

    // 构造函数，初始化单精度浮点数值
    // 参数v: 要封装的单精度浮点数值
    CCFloat(const float v){
        this->value = v;
    }

    // 将单精度浮点数值转换为字符串表示
    // 参数x: 小数点后的位数，默认为5
    // 返回值: 单精度浮点数值的字符串表示
    [[nodiscard]] StdString to_String(const int x = 5) const{
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(x) << value;
        return stream.str();
    }
    // 显式转换为单精度浮点数值
    operator float() const{
        return value;
    }
    operator StdString () const{
        return std::to_string(value);
    }
    bool operator == (const CCFloat& other) const{
        return value == other.value;
    }
    bool operator == (const float& other) const{
        return value == other;
    }
    bool operator != (const CCFloat& other) const{
        return value != other.value;
    }
    bool operator != (const float& other) const{
        return value != other;
    }
    bool operator > (const CCFloat& other) const{
        return value > other.value;
    }
    bool operator > (const float& other) const{
        return value > other;
    }
    bool operator < (const CCFloat& other) const{
        return value < other.value;
    }
    bool operator < (const float& other) const{
        return value < other;
    }
    bool operator >= (const CCFloat& other) const{
        return value >= other.value;
    }
    bool operator >= (const float& other) const{
        return value >= other;
    }
    bool operator <= (const CCFloat& other) const{
        return value <= other.value;
    }
    bool operator <= (const float& other) const{
        return value <= other;
    }
    float operator + (const CCFloat& other) const{
        return value + other.value;
    }
    float operator + (const float& other) const{
        return value + other;
    }
    float operator - (const CCFloat& other) const{
        return value - other.value;
    }
    float operator - (const float& other) const{
        return value - other;
    }
    float operator * (const CCFloat& other) const{
        return value * other.value;
    }
    float operator * (const float& other) const{
        return value * other;
    }
    float operator / (const CCFloat& other) const{
        return value / other.value;
    }
    float operator / (const float& other) const{
        return value / other;
    }
    float operator ++ (){
        return ++value;
    }
    float operator -- (){
        return --value;
    }
    float operator = (const CCFloat& other){
        value = other.value;
        return value;
    }
    float operator = (const float& other){
        value = other;
        return value;
    }
    float operator -= (const CCFloat& other){
        value -= other.value;
        return value;
    }
    float operator -= (const float& other){
        value -= other;
        return value;
    }
    float operator += (const CCFloat& other){
        value += other.value;
        return value;
    }
    float operator += (const float& other){
        value += other;
        return value;
    }
    float operator *= (const CCFloat& other){
        value *= other.value;
        return value;
    }
    float operator *= (const float& other){
        value *= other;
        return value;
    }
    float operator /= (const CCFloat& other){
        value /= other.value;
        return value;
    }
    float operator /= (const float& other){
        value /= other;
        return value;
    }
};

// CCSize类用于封装尺寸值（size_t类型）
class CCSize {
    size_t value; // 存储尺寸值的变量

public:
    // 默认构造函数
    CCSize() = default;

    // 构造函数，初始化尺寸值
    // 参数v: 要封装的尺寸值
    CCSize(const size_t v){
        this->value = v;
    }

    // 显式转换为尺寸值
    // 返回值: 封装的尺寸值
    operator size_t() const{
        return value;
    }

    // 将尺寸值转换为字符串表示
    // 返回值: 尺寸值的字符串表示
    [[nodiscard]] StdString to_String() const{
        return std::to_string(value);
    }
    operator StdString () const{
        return std::to_string(value);
    }
    bool operator == (const CCSize& other) const{
        return value == other.value;
    }
    bool operator == (const size_t& other) const{
        return value == other;
    }
    bool operator != (const CCSize& other) const{
        return value != other.value;
    }
    bool operator != (const size_t& other) const{
        return value != other;
    }
    bool operator > (const CCSize& other) const{
        return value > other.value;
    }
    bool operator > (const size_t& other) const{
        return value > other;
    }
    bool operator < (const CCSize& other) const{
        return value < other.value;
    }
    bool operator < (const size_t& other) const{
        return value < other;
    }
    bool operator >= (const CCSize& other) const{
        return value >= other.value;
    }
    bool operator >= (const size_t& other) const{
        return value >= other;
    }
    bool operator <= (const CCSize& other) const{
        return value <= other.value;
    }
    bool operator <= (const size_t& other) const{
        return value <= other;
    }
    size_t operator + (const CCSize& other) const{
        return value + other.value;
    }
    size_t operator + (const size_t& other) const{
        return value + other;
    }
    size_t operator - (const CCSize& other) const{
        return value - other.value;
    }
    size_t operator - (const size_t& other) const{
        return value - other;
    }
    size_t operator * (const CCSize& other) const{
        return value * other.value;
    }
    size_t operator * (const size_t& other) const{
        return value * other;
    }
    size_t operator / (const CCSize& other) const{
        return value / other.value;
    }
    size_t operator / (const size_t& other) const{
        return value / other;
    }
    size_t operator ++ (){
        return ++value;
    }
    size_t operator -- (){
        return --value;
    }
    size_t operator = (const CCSize& other){
        value = other.value;
        return value;
    }
    size_t operator = (const size_t& other){
        value = other;
        return value;
    }
    size_t operator -= (const CCSize& other){
        value -= other.value;
        return value;
    }
    size_t operator -= (const size_t& other){
        value -= other;
        return value;
    }
    size_t operator += (const CCSize& other){
        value += other.value;
        return value;
    }
    size_t operator += (const size_t& other){
        value += other;
        return value;
    }
    size_t operator *= (const CCSize& other){
        value *= other.value;
        return value;
    }
    size_t operator *= (const size_t& other){
        value *= other;
        return value;
    }
    size_t operator /= (const CCSize& other){
        value /= other.value;
        return value;
    }
    size_t operator /= (const size_t& other){
        value /= other;
        return value;
    }
};


#if __OHOS__
#pragma warning(pop)
// 或者对于 GCC/Clang
#pragma GCC diagnostic pop
#endif

#endif

extern CCAppInit_ CCAppInit;
