#ifndef CC_COOKIE_H
#define CC_COOKIE_H
#pragma once

#include "CCString.h"
#include "CCMap.h"
#include "CCVector.h"
#include <ctime>
#include <sstream>

namespace CTL {

/**
 * Cookie 数据结构，对应 HTTP Cookie 的完整属性
 */
struct Cookie {
    CTL::String Name;          ///< Cookie 名称
    CTL::String Value;         ///< Cookie 值
    CTL::String Domain;        ///< 生效域名（空 = 当前域名）
    CTL::String Path = "/";    ///< 生效路径
    time_t     Expires = 0;    ///< 过期时间（0 = 会话 Cookie，浏览器关闭即失效）
    bool       Secure   = false;   ///< 仅 HTTPS 传输
    bool       HttpOnly = false;   ///< 禁止 JavaScript 访问
    CTL::String SameSite;      ///< "Strict" / "Lax" / "None"
    uint32_t   MaxAge   = 0;   ///< 最大存活秒数（0 = 未设置，Expires 优先）

    /**
     * 构造一个空的会话 Cookie
     */
    Cookie() = default;

    /**
     * 构造一个基本的会话 Cookie
     * @param name Cookie 名称
     * @param value Cookie 值
     */
    Cookie(const CTL::String& name, const CTL::String& value)
        : Name(name), Value(value) {}

    /**
     * 构建 Set-Cookie 响应头字符串
     * @return 符合 RFC 6265 的 Set-Cookie 头值
     */
    [[nodiscard]] CTL::String ToSetCookieHeader() const {
        CTL::String header;
        header += Name + "=" + Value;

        if (!Domain.empty())
            header += "; Domain=" + Domain;
        if (!Path.empty())
            header += "; Path=" + Path;
        if (Expires > 0) {
            // 格式化为 RFC 1123: Wdy, DD Mon YYYY HH:MM:SS GMT
            char buf[128] = {};
            struct tm gmt{};
#if defined(_WIN32)
            gmtime_s(&gmt, &Expires);
#else
            gmtime_r(&Expires, &gmt);
#endif
            strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &gmt);
            header += "; Expires=" + CTL::String(buf);
        }
        if (MaxAge > 0)
            header += "; Max-Age=" + CTL::String(std::to_string(MaxAge));
        if (Secure)
            header += "; Secure";
        if (HttpOnly)
            header += "; HttpOnly";
        if (!SameSite.empty())
            header += "; SameSite=" + SameSite;

        return header;
    }

    /**
     * 快速创建一个会话 Cookie（仅 name=value）
     */
    static Cookie Session(const CTL::String& name, const CTL::String& value) {
        return Cookie(name, value);
    }

    /**
     * 快速创建一个持久化 Cookie
     * @param name  名称
     * @param value 值
     * @param maxAgeSeconds 存活秒数
     */
    static Cookie Persistent(const CTL::String& name, const CTL::String& value,
                             uint32_t maxAgeSeconds) {
        Cookie c(name, value);
        c.MaxAge = maxAgeSeconds;
        c.Expires = std::time(nullptr) + maxAgeSeconds;
        return c;
    }

    /**
     * 快速创建一个安全 HttpOnly 的持久化 Cookie（常用于 Token/会话）
     */
    static Cookie SecureToken(const CTL::String& name, const CTL::String& value,
                              uint32_t maxAgeSeconds) {
        Cookie c = Persistent(name, value, maxAgeSeconds);
        c.Secure   = true;
        c.HttpOnly = true;
        c.SameSite = "Strict";
        return c;
    }

    /**
     * 快速创建一个用于清除的 Cookie（设置过期时间为过去）
     */
    static Cookie Delete(const CTL::String& name) {
        Cookie c(name, "");
        c.MaxAge  = 0;
        c.Expires = 1; // Unix epoch 附近，已过期
        return c;
    }
};

/**
 * Cookie 解析工具类
 */
class CookieParser {
public:
    /**
     * 从 HTTP 请求的 Cookie 头解析所有 Cookie
     * @param cookieHeader "Cookie: name1=value1; name2=value2" 的值部分
     * @return 名称 -> Cookie 的映射
     */
    static CCMap<CTL::String, CTL::String> ParseRequestCookies(const CTL::String& cookieHeader) {
        CCMap<CTL::String, CTL::String> result;
        if (cookieHeader.empty()) return result;

        size_t pos = 0;
        while (pos < cookieHeader.size()) {
            // 跳过前导空格
            while (pos < cookieHeader.size() && cookieHeader[pos] == ' ')
                pos++;

            // 找 '='
            size_t eqPos = cookieHeader.find('=', pos);
            if (eqPos == CTL::String::npos) break;

            // 提取名称
            CTL::String name = cookieHeader.substr(pos, eqPos - pos);
            // trim trailing spaces in name
            while (!name.empty() && name.back() == ' ')
                name.pop_back();

            pos = eqPos + 1;

            // 找 ';' 或字符串结尾
            size_t semiPos = cookieHeader.find(';', pos);
            CTL::String value;
            if (semiPos == CTL::String::npos) {
                value = cookieHeader.substr(pos);
                pos = cookieHeader.size();
            } else {
                value = cookieHeader.substr(pos, semiPos - pos);
                pos = semiPos + 1;
            }

            // trim leading/trailing spaces in value
            while (!value.empty() && value.front() == ' ')
                value.erase(0, 1);
            while (!value.empty() && value.back() == ' ')
                value.pop_back();

            if (!name.empty()) {
                result[name] = URLDecodeCookieValue(value);
            }
        }
        return result;
    }

    /**
     * URL 解码 Cookie 值（处理 %xx 编码）
     */
    static CTL::String URLDecodeCookieValue(const CTL::String& in) {
        CTL::String out;
        for (size_t i = 0; i < in.size(); ++i) {
            if (in[i] == '%' && i + 2 < in.size()) {
                const char hi = in[i + 1];
                const char lo = in[i + 2];
                auto hexToDec = [](char c) -> int {
                    if (c >= '0' && c <= '9') return c - '0';
                    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                    return -1;
                };
                const int h = hexToDec(hi);
                const int l = hexToDec(lo);
                if (h >= 0 && l >= 0) {
                    out += static_cast<char>((h << 4) | l);
                    i += 2;
                    continue;
                }
            }
            out += in[i];
        }
        return out;
    }

    /**
     * 从 CCMap 头部集合中查找 Cookie 头并解析
     * @param headers 请求头映射（不区分大小写）
     * @return 名称 -> 值的映射
     */
    static CCMap<CTL::String, CTL::String> ParseFromHeaders(
            const CCMap<CTL::String, CTL::String>& headers) {
        // 尝试 "Cookie" 和 "cookie"
        const CTL::String key1("Cookie");
        const CTL::String key2("cookie");
        if (headers.find(key1) != headers.end()) {
            return ParseRequestCookies(headers.at(key1));
        }
        if (headers.find(key2) != headers.end()) {
            return ParseRequestCookies(headers.at(key2));
        }
        return {};
    }
};

} // namespace CTL

#endif // CC_COOKIE_H
