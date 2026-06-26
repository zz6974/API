#ifndef CCString_H
#define CCString_H

#include <sstream>
#include <cstdio>
#include <string>
#include "cstdarg"
#include "CCByteArray.h"
#include <locale>
#include <codecvt>
#include <sstream>
#include "basic/basic.h"

#ifdef __linux__
#include <iconv.h>
#endif

#if __OHOS__
#pragma warning(push)
#pragma warning(disable: -Wformat-security)
// 或者对于 GCC/Clang
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

using CCStream = std::ostringstream;
using CCSStream = std::stringstream;
using WString = std::wstring;
namespace CTL {
    class CCWString;
    /**
     * @brief 字符串类，继承自std::string，提供额外的功能
     */

    // C++11 类型特征辅助工具

    /**
     * @brief 字符串类，继承自 std::string，提供额外的功能
     */
    class String : public std::string{
    public:
        // 默认构造函数
        String() = default;
        template<typename... Args>
        String(const char* fmt, Args... args) {
            const auto str = format(fmt,args...);
            this->assign(str);
        }
        String(const std::initializer_list<std::string> list);
        explicit String(const CCSStream& stream);
        // 继承 std::string 的构造函数
        using std::string::string;
        // 转换字符串为字节
        [[nodiscard]] Byte* toBytes() const;
        [[nodiscard]] Byte* toBytes();
        // 从 basic_string 构造 String 对象
        String(const basic_string &basicString);
        // 格式化字符串
        String Encoder(const char * standardCharsets) const;
        String Encoder(const char * standardCharsets);
        // 指定大小构造字符串
        String(size_t Size);
        // 字符串转换为整数
        int to_int() const;
        float to_float() const;
        double to_double() const;
        // 字符串转换为整数（非 const 版本）
        int to_int();
        float to_float();
        double to_double();
        /**
         * @brief 格式化字符串的静态方法
         * @param fmt 格式化字符串的格式
         * @param args 可变参数，与格式化字符串匹配
         * @return 返回格式化后的 String 对象
         */
        template<typename... Args>
        static std::string format(const char* fmt, Args... args) {
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
        bool is_str_utf8() const;
        bool is_str_gbk() const;
        [[nodiscard]] WString to_CCWString() const {
#ifdef _WIN32
            try {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                const char* utf_8 = this->c_str();
                return converter.from_bytes(utf_8);
            }
            catch (...) {
                return L"";
            }
#elif __linux__
            iconv_t cd = iconv_open("UTF-8", "WCHAR_T");
            if (cd == (iconv_t)-1) {
                return L"";
            }
            size_t inBytes = size() * sizeof(wchar_t);
            size_t outBytes = inBytes * 2;
            char* inbuf = (char*)data();
            char* outbuf = new char[outBytes];
            char* outbufEnd = outbuf + outBytes;
            size_t convertedBytes = iconv(cd, &inbuf, &inBytes, &outbuf, &outBytes);
            iconv_close(cd);
            if (convertedBytes == (size_t)-1) {
                delete[] outbuf;
                return L"";
            }
            auto* wcBuf = reinterpret_cast<wchar_t*>(outbuf);
            size_t wcLength = (outbufEnd - outbuf) / sizeof(wchar_t);
            WString result(wcBuf, wcLength);  // 正确调用 wchar_t 数组构造
            delete[] outbuf;
            return result;
#endif
        }
        String& operator<<(const char* str) {
            this->append(str);
            return *this;
        }
        String& operator<<(const int value) {
            this->append(std::to_string(value));
            return *this;
        }
        String& operator<<(const float value) {
            this->append(std::to_string(value));
            return *this;
        }
        String& operator<<(const double value) {
            this->append(std::to_string(value));
            return *this;
        }
        String& operator<<(const String& other) {
            this->append(other);
            return *this;
        }
        static std::vector<std::string> split(const std::string& str, const std::string& delimiter);
        [[nodiscard]] std::vector<std::string> split(const std::string& str) const;
        String operator=(const String& other);
        String to_gbk() const;
        String to_utf8() const;
    };


    /**
     * @brief 宽字符串类，继承自std::wstring，提供额外的功能
     */
    class CCWString:public WString{
    public:
        // 默认构造函数
        CCWString() = default;
        // 从std::wstring构造CCWString对象
        CCWString(const std::wstring& wstr) {
            assign(wstr);
        }
        // 转换CCWString对象为String对象
        [[nodiscard]] String to_CCString() const {
#ifdef _WIN32
            try {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                return converter.to_bytes(data());
            }
            catch (...) {
                return "";
            }
#elif __linux__
                iconv_t cd = iconv_open("UTF-8", "WCHAR_T"); // 修正转换方向
                if (cd == (iconv_t)-1) return "";
                
                size_t inBytes = size() * sizeof(wchar_t);
                char* inbuf = (char*)data();
                size_t outBytes = inBytes * 4;  // 扩大输出缓冲区
                char* outbuf = new char[outBytes];
                char* outptr = outbuf;
                
                if (iconv(cd, &inbuf, &inBytes, &outptr, &outBytes) == (size_t)-1) {
                    delete[] outbuf;
                    iconv_close(cd);
                    return "";
                }
                iconv_close(cd);
                
                // 使用转换后的char数组构造String
                String result(outbuf, outptr - outbuf);
                delete[] outbuf;
                return result;
#endif
        }
    };

}

inline CTL::String& operator<<(const char* lhs, CTL::String& rhs) {
    rhs.append(lhs);
    return rhs;
}

// 为CTL::String类型提供哈希函数
// template <>
// struct std::hash<CTL::String> {
//     std::size_t operator()(const CTL::String& s) const noexcept {
//         return std::hash<std::string>()(s);
//     }
// };

#if __OHOS__
#pragma warning(pop)
// 或者对于 GCC/Clang
#pragma GCC diagnostic pop
#endif

#endif
