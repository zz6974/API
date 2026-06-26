#ifndef HttpClient_H
#define HttpClient_H
#pragma once

// 包含必要的头文件
#include "../Module/Comm/Socket/CCSocket.h"
#include "CCJSONObject.h"
#include "CCString.h"
#include <sstream>
#include <regex>
#include "map"
#include "CCRequest.h"
#include "CCClientSocket.h"
#include "CCHttpResolver.h"

// 定义最大数据大小
#define MaxDataSize 2048

// 命名空间CTL开始
namespace CTL {
    class Result {
    public:
        Result() = default;
        Result(int status_t, const std::string &reason);
        std::string version;
        int status = -1;
        std::string reason;
        std::map<String, String> Headers;
        std::string body;
        std::string location; // Redirect location
    };
    class ReqConfig {
    public:
        String method; // 请求方法
        String path; // 请求路径
        ByteArray body; // 请求体
        String type; // 请求体类型
    };
    class HttpClient{
        std::string httpUrl;
        std::string ip;
        int port = 0;
        bool is_https = false;
        std::map<String, String> Headers;
        ClientSocket* socket = nullptr;
        mutable SSL* ssl = nullptr;
        SSL_CTX* ctx = nullptr;
        int m_timeout = 30;  // 连接/读写超时（秒）
        bool m_followRedirects = true;

        using onResCallback = std::function<void(const CCHttpResolver&)>;
        using onStreamCallback = std::function<bool(const char* data, size_t len)>;
        using onProgressCallback = std::function<void(size_t received, size_t total)>;
    public:
        explicit HttpClient(const std::string& httplink);
        ~HttpClient();

        // ---- 配置 ----
        void setHeader(const std::string& key, const std::string& value);
        void setTimeout(int seconds) { m_timeout = seconds; }
        void setFollowRedirects(bool follow) { m_followRedirects = follow; }

        // ---- 通用请求 ----
        Result Request(ReqConfig& req, const onResCallback& ResCallback, const onStreamCallback& callback);
        Result Request(ReqConfig& req);

        // ---- 流式接收 ----
        Result RequestStream(ReqConfig& req, const onStreamCallback& onChunk);
        Result RequestStream(ReqConfig& req, const onResCallback& onHeader, onStreamCallback onChunk);

        // ---- JSON / 文本 便捷方法 ----
        Result RequestJson(ReqConfig& req, std::string& outBody);

        // ---- 文件下载 ----
        Result RequestDownload(ReqConfig& req, const std::string& savePath,
            onProgressCallback onProgress = nullptr);

        // ---- 文件上传 ----
        Result RequestUpload(ReqConfig& req, const std::string& filePath,
            const std::string& fileFieldName = "file",
            const std::map<std::string, std::string>& extraFields = {},
            onProgressCallback onProgress = nullptr);

        // ========== 快捷请求方法 ==========
        Result Get(const std::string& path = "/",
                   const onResCallback& onHeader = nullptr,
                   const onStreamCallback& onChunk = nullptr);

        Result Post(const std::string& path,
                    const std::string& body,
                    const std::string& contentType = "application/json");

        Result Put(const std::string& path,
                   const std::string& body,
                   const std::string& contentType = "application/json");

        Result Delete(const std::string& path,
                      const std::string& body = "");

        Result Patch(const std::string& path,
                     const std::string& body,
                     const std::string& contentType = "application/json");

        Result Head(const std::string& path = "/");

        Result Options(const std::string& path = "/");

    private:
        bool parseURL(const std::string& url);
        bool _connect();
        void _disconnect();
        String GetRequest() const;
        String GetRequestAll() const;
        int Read(char* buffer, size_t length) const;
        Result Write(const String& str) const;
        Result Write(const ByteArray& str) const;
        int read_t(char* buffer, size_t length) const;
        String getText(size_t len) const;
        // ---- 内部辅助 ----
        Result sendAndReceiveHeader(ReqConfig& req, CCHttpResolver& resolver);
        Result _doRequest(ReqConfig& req);
    };
}

// 命名空间CTL结束
#endif
