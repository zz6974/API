#ifndef CCWeb_Response_H
#define CCWeb_Response_H
#pragma once

// 检查宏定义
#ifdef Status
#undef Status
#endif

#include "CCRequest.h"
#include "CCCookie.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "TL/Queue.h"

/**
 * @namespace CTL
 * @brief 该命名空间包含处理 Web 请求和响应的类和函数。
 */
namespace CTL {
    /**
     * @class Response
     * @brief 该类用于处理 HTTP 响应，包括向客户端发送文件、资源和自定义内容。
     */
    class Response{
    public:
        ~Response();

        /**
         * @brief 初始化响应对象。
         *
         * @param channel 客户端套接字。
         * @param cors 跨域资源共享 (CORS) 设置，如果不需要 CORS 可以传入 nullptr。
         */
        void Init(HttpSocketChannel* channel, CORS* cors);

        /**
         * @brief 设置响应的路由地址。
         *
         * @param url 路由地址。
         */
        void setUrl(const String& url);

        /**
         * @brief 向客户端发送文件作为响应。
         *
         * @param Path 文件路径。
         * @param Status_t 发送的 HTTP 状态码，默认为 200。
         * @return 如果文件发送成功返回 true，否则返回 false。
         */
        bool ResponseFile(const CTL::String& Path, int Status_t = 200);

        /**
         * @brief 向客户端发送资源作为响应。
         *
         * @param Data 资源数据。
         * @param Status_t 发送的 HTTP 状态码，默认为 200。
         * @return 如果资源发送成功返回 true，否则返回 false。
         */
        bool Resource(const CCVector<char>& Data, int Status_t = 200);

        /**
         * @brief 向客户端发送可下载的文件资源。
         *
         * @param Path 文件路径。
         * @param SendBufferSize 发送缓冲区大小，默认为 4096 字节。
         * @return 如果文件发送成功返回 true，否则返回 false。
         */
        bool ResourceDownload(const CTL::String& Path, int SendBufferSize = 4096);

        /**
         * @brief 设置响应头信息。
         *
         * @param Key 头信息键。
         * @param value 头信息值。
         */
        void SetHeader(const CTL::String& Key, const CTL::String& value);

        /**
         * @brief 向客户端写入字符串数据。
         *
         * @param Data 要发送的数据。
         * @return 如果数据发送成功返回 true，否则返回 false。
         */
        bool Write(const CTL::String& Data) const;

        /**
         * @brief 向客户端发送原始数据。
         *
         * @param buffer 数据缓冲区。
         * @param size 缓冲区大小。
         * @return 如果数据发送成功返回 true，否则返回 false。
         */
        bool Flash(const char* buffer, size_t size) const;

        /**
         * @brief 向客户端写入字符向量数据，并设置状态码。
         *
         * @param Data 要发送的数据。
         * @param Status_t 发送的 HTTP 状态码。
         * @return 如果数据发送成功返回 true，否则返回 false。
         */
        bool Write(const CCVector<char>& Data, int Status_t);

        /**
         * @brief 向客户端写入字符串数据，并设置状态码（不强制关闭连接，用于 SSE 等长连接场景）。
         *
         * @param Data 要发送的数据。
         * @param Status_t 发送的 HTTP 状态码。
         * @return 如果数据发送成功返回 true，否则返回 false。
         */
        bool WriteStatus(const CTL::String& Data, int Status_t);

        /**
         * @brief 向客户端写入字符串数据，并设置状态码。
         *
         * @param Data 要发送的数据。
         * @param Status_t 发送的 HTTP 状态码。
         * @return 如果数据发送成功返回 true，否则返回 false。
         */
        bool Write(const CTL::String& Data, int Status_t);

        /**
         * @brief 处理路由响应。
         */
        void ResponseRouting() const;

        /**
         * @brief 写入响应头信息。
         *
         * @param Status_t HTTP 状态码。
         * @param ContentType 内容类型。
         */
        void WriteHeader(int Status_t, const CTL::String& ContentType);

        /**
         * @brief 处理范围请求的媒体文件。
         *
         * @param request 请求对象。
         * @param path 文件路径。
         * @param Content_Type 内容类型，默认为 "-.-"。
         */
        void RangeMediaFile(Request request, const CTL::String& path, CTL::String Content_Type = "-.-");

        /**
         * @brief 让浏览器重新访问另一个URL。
         *
         * @param url 对象链接。
         */
        void SendRedirect(const String& url);

        /**
         * @brief 获取响应头信息。
         *
         * @param key 头信息键。
         * @return 头信息值。
         */
        String getHeader(const String& key);

        /**
         * @brief 块数据传输。
         *
         * @param func 数据传输回调函数。
         */
        void BlockTransmission(const std::function<bool(CTL::ByteArray* buff)>& func);

        /**
        * @brief 初始化 multipart/form-data 响应，生成边界字符串。
        * @return Response& 支持链式调用。
        */
        Response& Multipart();

        /**
         * @brief 添加一个普通表单字段。
         * @param name 字段名。
         * @param value 字段值。
         * @return Response& 支持链式调用。
         */
        Response& MultipartAdd(const CTL::String& name, const CTL::String& value);

        /**
         * @brief 添加一个文件字段（从磁盘路径读取）。
         * @param name 字段名。
         * @param filePath 文件路径，MIME 类型自动从扩展名推断。
         * @return Response& 支持链式调用。
         */
        Response& MultipartAddFile(const CTL::String& name, const CTL::String& filePath);

        /**
         * @brief 添加一个文件字段（从内存数据）。
         * @param name 字段名。
         * @param filename 文件名。
         * @param data 文件二进制数据。
         * @param mimeType 文件的 MIME 类型。
         * @return Response& 支持链式调用。
         */
        Response& MultipartAddFile(const CTL::String& name, const CTL::String& filename,
                                   const CTL::ByteArray& data, const CTL::String& mimeType);

        /**
         * @brief 添加一个自定义 Part（原始数据 + 自定义 Content-Type）。
         * @param name 字段名。
         * @param data 数据。
         * @param contentType 内容类型，如 "application/json"。
         * @return Response& 支持链式调用。
         */
        Response& MultipartAdd(const CTL::String& name, const CTL::ByteArray& data,
                               const CTL::String& contentType);

        /**
         * @brief 构建并发送完整的 multipart 响应。
         */
        void MultipartSend();

        // ==================== Cookie 相关 ====================

        /**
         * @brief 设置一个 Cookie
         * @param cookie Cookie 对象
         */
        void SetCookie(const Cookie& cookie);

        /**
         * @brief 设置一个简单的会话 Cookie
         * @param name Cookie 名称
         * @param value Cookie 值
         */
        void SetCookie(const CTL::String& name, const CTL::String& value);

        /**
         * @brief 设置一个持久化 Cookie
         * @param name Cookie 名称
         * @param value Cookie 值
         * @param maxAgeSeconds 存活秒数
         * @param httpOnly 是否 HttpOnly
         * @param secure 是否 Secure
         */
        void SetCookie(const CTL::String& name, const CTL::String& value,
                       uint32_t maxAgeSeconds, bool httpOnly = false, bool secure = false);

        /**
         * @brief 删除指定名称的 Cookie（通过设置过期时间为过去）
         * @param name Cookie 名称
         * @param path Cookie 路径（需与设置时一致，默认 "/"）
         */
        void RemoveCookie(const CTL::String& name, const CTL::String& path = "/");

        // ==================== Auth 相关 ====================

        /**
         * @brief 发送 401 Unauthorized，要求客户端提供 Basic 认证
         * @param realm 认证域描述字符串
         */
        void RequireBasicAuth(const CTL::String& realm = "Restricted Area");

        /**
         * @brief 发送 401 Unauthorized，要求客户端提供 Bearer Token
         * @param realm 认证域描述字符串
         */
        void RequireBearerAuth(const CTL::String& realm = "Restricted Area");

        /**
         * @brief 发送 403 Forbidden
         * @param message 错误信息
         */
        void SendForbidden(const CTL::String& message = "Forbidden");

        /**
         * @brief 发送 401 Unauthorized
         * @param message 错误信息
         */
        void SendUnauthorized(const CTL::String& message = "Unauthorized");
    private:
        /**
         * @brief 发送响应头信息。
         *
         * @param Status_t HTTP 状态码。
         * @param HeaderFlag HTTP 头 末尾标志。
         */
        void SendHeader(int Status_t, const CTL::String& HeaderFlag = "OK");
        /**
         * @brief 发送响应头信息。
         *
         * @param Status_t HTTP 状态码。
         * @param HeaderFlag HTTP 头 末尾标志。
         */
        String GetHeader(int Status_t, const CTL::String& HeaderFlag = "OK");

    private:
        /**
         * @brief 流式 Multipart 文件分段信息。
         */
        struct MpFilePart {
            CTL::String filePath;
            CTL::String fileName;
            CTL::String mimeType;
        };

        CTL::String    _mpBoundary;
        CTL::ByteArray _mpBody;
        CCVector<MpFilePart> _mpFileParts;   // 磁盘文件引用列表
        static CCMutex _mutex;
        // 文件头信息映射表。
        CCMap<CTL::String, CTL::String> FileHeaders;

        // 自定义响应头信息映射表。
        CCMap<CTL::String, CTL::String> Headers;

        // 客户端套接字。
        HttpSocketChannel* channel = nullptr;
        // CORS 设置。
        CORS* CORS_ = nullptr;
        String m_url;
    public:

    };
}


#endif
