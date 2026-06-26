#include "CCHttpClient.h"
#include "CCSystem.h"
#include <fstream>

using namespace CTL;

bool SSL_available(SSL* ssl) {
    if (ssl) {
        if (SSL_pending(ssl) > 0) {
            return true;
        }
        int sockfd = SSL_get_fd(ssl);
        if (sockfd >= 0) {
            fd_set readfds;
            struct timeval timeout = {0, 0};
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            int result = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
            return result > 0;
        }
    }
    return false;
}

void cleanupSSL(SSL*& ssl, SSL_CTX*& ctx) {
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
    }
    if (ctx) {
        SSL_CTX_free(ctx);
        ctx = nullptr;
    }
}

// ========== 内部辅助：统一 TCP + SSL 连接 ==========
bool HttpClient::_connect() {
    if (socket) { _disconnect(); }
    socket = new ClientSocket(InetAddress(this->ip), this->port);
    if (!socket->isConnect()) {
        delete socket; socket = nullptr;
        return false;
    }
    if (!this->is_https) return true;

    // --- SSL 握手 ---
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        delete socket; socket = nullptr;
        return false;
    }
    ssl = SSL_new(ctx);
    SSL_set_tlsext_host_name(ssl, this->ip.c_str());
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);
    SSL_CTX_set_default_verify_paths(ctx);
    if (!ssl) {
        SSL_CTX_free(ctx); ctx = nullptr;
        delete socket; socket = nullptr;
        return false;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    SSL_set_fd(ssl, socket->getSocket()->Socketbit);
    if (SSL_connect(ssl) <= 0) {
        char err_buf[256] = {0};
        ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
        std::cerr << "SSL handshake failed: " << err_buf << std::endl;
        SSL_free(ssl); ssl = nullptr;
        SSL_CTX_free(ctx); ctx = nullptr;
        delete socket; socket = nullptr;
        return false;
    }
    return true;
}

void HttpClient::_disconnect() {
    if (ssl) { SSL_shutdown(ssl); SSL_free(ssl); ssl = nullptr; }
    if (ctx) { SSL_CTX_free(ctx); ctx = nullptr; }
    if (socket) { socket->close(); delete socket; socket = nullptr; }
}

Result::Result(const int status_t, const std::string &reason) {
    this->status = status_t;
    this->reason = reason;
}

HttpClient::HttpClient(const std::string& httplink){
    this->httpUrl = httplink;
    const bool F = parseURL(this->httpUrl);
    if (F) {
        CTL::String ip_port = this->ip;
        if (this->port != 0) {
            ip_port = String::format("{}:{}",this->ip.c_str(),this->port);
        }
        Headers["User-Agent"] = "CC_SDK_httpClient/7.41.2";
        Headers["Accept"] = "*/*";
        Headers["Host"] = ip_port;
        Headers["Accept-Encoding"] = "";
        Headers["Connection"] = "keep-alive";
    }
    else {
        throw std::invalid_argument("Invalid URL format");
    }
}

HttpClient::~HttpClient(){
    _disconnect();
}

void HttpClient::setHeader(const std::string &key, const std::string &value) {
    Headers[key] = value;
}

// ========== 通用请求（带回调，流式读取响应体） ==========
Result HttpClient::Request(ReqConfig& req, const onResCallback& ResCallback, const onStreamCallback& callback) {
    if (!_connect()) return Result(-1, "Failed to connect to server");

    const String requestUrl = req.path.empty() ? this->httpUrl : req.path;
    Result result;

    // 组装请求头
    String request = req.method + " " + requestUrl + " HTTP/1.1\r\n";
    if (!req.type.empty()) {
        setHeader("Content-Type", req.type);
        setHeader("Content-Length", std::to_string(req.body.size()));
    }
    for (const auto& header : Headers) {
        request += header.first + ":" + header.second + "\r\n";
    }
    request += "\r\n";

    // 发送请求头
    auto ret = Write(request);
    if (ret.status < 0) { _disconnect(); return ret; }

    // 发送请求体
    if (!req.body.IsEmpty()) {
        ret = Write(req.body);
        if (ret.status < 0) { _disconnect(); return ret; }
    }

    // 读取响应头
    const String response = GetRequest();
    if (response.empty()) {
        _disconnect();
        return Result(-1, "Empty response received");
    }

    CCHttpResolver resolver;
    resolver.tryDecodeResponse(response);  // 用 tryDecodeResponse 解析响应

    if (ResCallback) ResCallback(resolver);

    // 流式读取响应体
    if (callback) {
        while (true) {
            char buffer[2048]{};
            const auto length = Read(buffer, sizeof buffer);
            if (length > 0) {
                if (!callback(buffer, length)) break;
            } else break;
        }
    }

    _disconnect();

    result.reason = response;
    result.status = resolver.getStatusCode();
    result.body = resolver.getBody();
    result.Headers = resolver.getHeaders();
    result.version = resolver.getVersion();
    return result;
}

// ========== 通用请求（简单版，读取全部响应） ==========
Result HttpClient::Request(ReqConfig& req) {
    if (!_connect()) return Result(-1, "Failed to connect to server");

    const String requestUrl = req.path.empty() ? this->httpUrl : req.path;
    Result result;

    // 组装请求头
    String request = req.method + " " + requestUrl + " HTTP/1.1\r\n";
    if (!req.type.empty()) {
        setHeader("Content-Type", req.type);
        setHeader("Content-Length", std::to_string(req.body.size()));
    }
    for (const auto& header : Headers) {
        request += header.first + ":" + header.second + "\r\n";
    }
    request += "\r\n";

    // 发送
    auto ret = Write(request);
    if (ret.status < 0) { _disconnect(); return ret; }
    if (!req.body.IsEmpty()) {
        ret = Write(req.body);
        if (ret.status < 0) { _disconnect(); return ret; }
    }

    // 读取全部响应
    const String response = GetRequestAll();
    _disconnect();

    if (response.empty()) return Result(-1, "Empty response received");

    CCHttpResolver resolver;
    resolver.tryDecodeResponse(response);

    result.reason = response;
    result.status = resolver.getStatusCode();
    result.body = resolver.getBody();
    result.Headers = resolver.getHeaders();
    result.version = resolver.getVersion();
    return result;
}

Result HttpClient::RequestStream(ReqConfig& req, const onStreamCallback& onChunk) {
    return RequestStream(req, nullptr, onChunk);
}

Result HttpClient::RequestStream(ReqConfig& req, const onResCallback& onHeader, onStreamCallback onChunk) {
    CCHttpResolver resolver;
    Result ret = sendAndReceiveHeader(req, resolver);
    if (ret.status < 0) return ret;

    const std::string headerStr = ret.reason;

    if (onHeader) onHeader(resolver);

    auto cl = resolver.getHeaders().find("Content-Length");
    size_t contentLength = 0;
    if (cl != resolver.getHeaders().end()) {
        contentLength = strtoul(cl->second.c_str(), nullptr, 10);
    }

    size_t totalReceived = 0;
    int noDataCount = 0;
    static const int NO_DATA_TIMEOUT = 500;

    while (true) {
        char buffer[8192];
        int len = read_t(buffer, sizeof(buffer));

        if (len > 0) {
            noDataCount = 0;
            totalReceived += len;
            std::vector<char> chunk(buffer, buffer + len);
            resolver.tryDecode2(headerStr, chunk, onChunk);

            if (resolver.isStream2Complete()) break;
            if (contentLength > 0 && totalReceived >= contentLength) break;
        } else if (len == -2) {
            break;
        } else {
            noDataCount++;
            if (noDataCount >= NO_DATA_TIMEOUT) break;
            Thread::Sleep(10);
        }
    }

    _disconnect();

    Result result;
    result.status = resolver.getStatusCode();
    result.reason = resolver.getReasonPhrase();
    result.Headers = resolver.getHeaders();
    result.version = resolver.getVersion();
    return result;
}

Result HttpClient::RequestJson(ReqConfig& req, std::string& outBody) {
    outBody.clear();

    auto ret = RequestStream(req, [&outBody](const char* data, size_t len) -> bool {
        outBody.append(data, len);
        return true;
    });

    if (ret.status < 0) return ret;

    ret.body = outBody;
    return ret;
}

Result HttpClient::RequestDownload(ReqConfig& req, const std::string& savePath, onProgressCallback onProgress) {
    std::ofstream file(savePath, std::ios::binary);
    if (!file.is_open()) {
        return Result(-1, "Failed to open file: " + savePath);
    }

    CCHttpResolver resolver;
    Result ret = sendAndReceiveHeader(req, resolver);
    if (ret.status < 0) {
        file.close();
        return ret;
    }

    const std::string headerStr = ret.reason;

    auto cl = resolver.getHeaders().find("Content-Length");
    size_t totalSize = 0;
    if (cl != resolver.getHeaders().end()) {
        totalSize = strtoul(cl->second.c_str(), nullptr, 10);
    }

    size_t received = 0;
    int noDataCount = 0;
    static const int NO_DATA_TIMEOUT = 1000;

    auto chunkCb = [&](const char* data, size_t len) -> bool {
        file.write(data, len);
        received += len;
        if (onProgress) onProgress(received, totalSize);
        return true;
    };

    while (true) {
        char buffer[8192];
        int len = read_t(buffer, sizeof(buffer));

        if (len > 0) {
            noDataCount = 0;
            std::vector<char> chunk(buffer, buffer + len);
            resolver.tryDecode2(headerStr, chunk, chunkCb);

            if (resolver.isStream2Complete()) break;
            if (totalSize > 0 && received >= totalSize) break;
        } else if (len == -2) {
            break;
        } else {
            noDataCount++;
            if (noDataCount >= NO_DATA_TIMEOUT) break;
            Thread::Sleep(10);
        }
    }

    file.close();
    _disconnect();

    Result result;
    result.status = resolver.getStatusCode();
    result.reason = resolver.getReasonPhrase();
    result.Headers = resolver.getHeaders();
    result.version = resolver.getVersion();
    return result;
}

Result HttpClient::RequestUpload(ReqConfig& req, const std::string& filePath, const std::string& fileFieldName,
    const std::map<std::string, std::string>& extraFields, onProgressCallback onProgress) {
    // 1. 读取文件
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return Result(-1, "Failed to open file: " + filePath);
    const size_t fileSize = file.tellg();
    file.seekg(0);
    std::vector<char> fileData(fileSize);
    file.read(fileData.data(), fileSize);
    file.close();

    // 2. 提取文件名
    std::string fileName = filePath;
    auto slashPos = fileName.find_last_of("/\\");
    if (slashPos != std::string::npos) fileName = fileName.substr(slashPos + 1);

    // 3. 生成边界
    std::string boundary = "----CCSDKBoundary" + std::to_string(rand());

    // 4. 构建 multipart body
    auto makeStr = [](const char* data, size_t len) {
        return std::string(data, len);
    };

    std::string prefix =
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"" + fileFieldName +
        "\"; filename=\"" + fileName + "\"\r\n"
        "Content-Type: application/octet-stream\r\n\r\n";

    std::string suffix = "\r\n";
    for (const auto& f : extraFields) {
        suffix += "--" + boundary + "\r\n";
        suffix += "Content-Disposition: form-data; name=\"" + f.first + "\"\r\n\r\n";
        suffix += f.second + "\r\n";
    }
    suffix += "--" + boundary + "--\r\n";

    ByteArray multipartBody;
    multipartBody.append(prefix);
    multipartBody.append(makeStr(fileData.data(), fileData.size()));
    multipartBody.append(suffix);

    if (onProgress) onProgress(fileSize, fileSize);

    // 5. 连接服务器
    if (!_connect()) return Result(-1, "Failed to connect to server");

    const String requestUrl = req.path.empty() ? this->httpUrl : req.path;

    // 6. 设置 multipart 头并发送请求
    setHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    setHeader("Content-Length", std::to_string(multipartBody.size()));

    String request = req.method + " " + requestUrl + " HTTP/1.1\r\n";
    for (const auto& header : Headers) {
        request += header.first + ":" + header.second + "\r\n";
    }
    request += "\r\n";

    auto ret = Write(request);
    if (ret.status < 0) { _disconnect(); return ret; }

    ret = Write(multipartBody);
    if (ret.status < 0) { _disconnect(); return ret; }

    // 7. 读取响应头
    const String headerStr = GetRequest();
    if (headerStr.empty()) {
        _disconnect();
        return Result(-1, "Empty response received");
    }
    CCHttpResolver resolver;
    resolver.tryDecodeResponse(headerStr);

    // 8. 流式读取响应体
    std::string responseBody;
    auto cl = resolver.getHeaders().find("Content-Length");
    size_t contentLength = 0;
    if (cl != resolver.getHeaders().end()) contentLength = strtoul(cl->second.c_str(), nullptr, 10);

    size_t totalReceived = 0;
    int noDataCount = 0;
    static const int NO_DATA_TIMEOUT = 500;

    while (true) {
        char buffer[8192];
        int len = read_t(buffer, sizeof(buffer));
        if (len > 0) {
            noDataCount = 0;
            totalReceived += len;
            std::vector<char> chunk(buffer, buffer + len);
            resolver.tryDecode2(headerStr, chunk,
                [&responseBody](const char* data, size_t size) -> bool {
                    responseBody.append(data, size);
                    return true;
                });
            if (resolver.isStream2Complete()) break;
            if (contentLength > 0 && totalReceived >= contentLength) break;
        } else if (len == -2) {
            break;
        } else {
            noDataCount++;
            if (noDataCount >= NO_DATA_TIMEOUT) break;
            Thread::Sleep(10);
        }
    }

    _disconnect();

    Result result;
    result.status = resolver.getStatusCode();
    result.reason = resolver.getReasonPhrase();
    result.Headers = resolver.getHeaders();
    result.version = resolver.getVersion();
    result.body = responseBody;
    return result;
}

bool HttpClient::parseURL(const std::string& url){
    std::regex url_regex(R"((http|https)://([^:/\s]+)(:([0-9]+))?(/.*|.*))");
    std::smatch match_results;
    if (std::regex_match(url, match_results, url_regex)) {
        ip = match_results[2];
        is_https = match_results[1] == "https";
        if (match_results[4].matched) {
            port = std::stoi(match_results[4]);
        }
        else {
            port = is_https ? 443 : 80;
        }
        return true;
    }
    else {
        std::cerr << "Invalid URL format: " << url << std::endl;
        ip = "";
        port = 0;
        return false;
    }
}

String HttpClient::GetRequest() const {
    if(ssl) {
        String data;
        data.reserve(4096);
        while (true){
            char buffer[CC_R_BUFFER_SIZE] = {0};
            const auto length = SSL_read(ssl, buffer, CC_R_BUFFER_SIZE);
            if(length > 0){
                data.append(buffer, length);
                const auto pos = data.find("\r\n\r\n");
                if (pos != CTL::String::npos) {
                    data.resize(pos + 4);
                    break;
                }
            }
            else{
                ERR_print_errors_fp(stderr);
                return "";
            }
        }
        return data;
    }
    else {
        CTL::String data;
        data.reserve(4096);
        int ET = 1;
        const auto input = socket->getInputStream();
        while (true){
            if(input->available()){
                char buffer[CC_R_BUFFER_SIZE] = {0};
                const auto length = input->read(buffer, CC_R_BUFFER_SIZE);
                if(length > 0){
                    data.append(buffer, length);
                    const auto pos = data.find("\r\n\r\n");
                    if (pos != CTL::String::npos) {
                        data.resize(pos + 4);
                        break;
                    }
                }
                else{
                    return "";
                }
            }
            else{
                if (ET >= 1000 * 100) {
                    break;
                }
                ET++;
                Thread::Sleep(10);
            }
        }
        return data;
    }
}

int HttpClient::Read(char *buffer, size_t length) const {
    if (this->is_https) {
        return SSL_read(ssl, buffer, length);
    }
    else {
        return socket->getInputStream()->read(buffer, length);
    }
}

Result HttpClient::Write(const String &str) const {
    if (ssl) {
        if (SSL_write(ssl, str.c_str(), str.length()) <= 0) {
            ERR_print_errors_fp(stderr);
            return Result(-1, "Failed to send HTTPS request");
        }
    }
    else {
        const auto out = socket->getOutputStream();
        if (!out->write(str.c_str(), str.length())) {
            return Result(-1, "Failed to send HTTP request");
        }
    }
    return Result(1, "Success");
}

Result HttpClient::Write(const ByteArray &str) const {
    if (ssl) {
        if (SSL_write(ssl, str.buffer(), str.size()) <= 0) {
            ERR_print_errors_fp(stderr);
            return Result(-1, "Failed to send HTTPS request");
        }
    }
    else {
        const auto out = socket->getOutputStream();
        if (!out->write(str.buffer(), str.size())) {
            return Result(-1, "Failed to send HTTP request");
        }
    }
    return Result(1, "Success");
}

int HttpClient::read_t(char* buffer, size_t length) const {
    if(ssl) {
        if (SSL_available(ssl)) {
            const auto len = SSL_read(ssl, buffer, length);
            if(len > 0){
                return len;
            }
            else{
                ERR_print_errors_fp(stderr);
                return -2;
            }
        }
        else{
            return -1;
        }
    }
    else {
        const auto input = socket->getInputStream();
        if(input->available()){
            const auto len = input->read(buffer, length);
            if(len > 0){
                return len;
            }
            else{
                return -2;
            }
        }
        else{
            return -1;
        }
    }
}

String HttpClient::getText(const size_t len) const {
    String data;
    size_t l = 0;
    int ET = 0;
    while (true) {
        char buffer[2048] = {0};
        const auto length = read_t(buffer, sizeof buffer);
        if (length > 0) {
            data.append(buffer, length);
            l += length;
            if (l >= len) {
                break;
            }
        }
        else {
            if (ET >= 1000 * 100) {
                break;
            }
            ET++;
            Thread::Sleep(10);
        }
    }
    return data;
}

Result HttpClient::sendAndReceiveHeader(ReqConfig& req, CCHttpResolver& resolver) {
    if (!_connect()) return Result(-1, "Failed to connect to server");

    const String requestUrl = req.path.empty() ? this->httpUrl : req.path;

    // 组装请求
    String request = req.method + " " + requestUrl + " HTTP/1.1\r\n";
    if (!req.type.empty()) {
        setHeader("Content-Type", req.type);
        setHeader("Content-Length", std::to_string(req.body.size()));
    }
    for (const auto& header : Headers) {
        request += header.first + ":" + header.second + "\r\n";
    }
    request += "\r\n";

    auto ret = Write(request);
    if (ret.status < 0) return ret;

    if (!req.body.IsEmpty()) {
        ret = Write(req.body);
        if (ret.status < 0) return ret;
    }

    // 读取响应头
    const String headerStr = GetRequest();
    if (headerStr.empty()) return Result(-1, "Empty response received");

    // 解析响应头
    resolver.tryDecodeResponse(headerStr);

    return Result(1, headerStr);
}

String HttpClient::GetRequestAll() const {
    if(ssl) {
        String data;
        data.reserve(4096);
        int ET = 1;
        while (true){
            if (SSL_available(ssl)) {
                char buffer[2048] = {0};
                const auto length = SSL_read(ssl, buffer, 2048);
                if(length > 0){
                    ET = 1;
                    data.append(buffer, length);
                }
                else{
                    ERR_print_errors_fp(stderr);
                    break;
                }
            }
            else{
                if (ET >= 1000 * 100) {
                    break;
                }
                ET++;
                Thread::SleepMS(1);
            }
        }
        return data;
    }
    else {
        CTL::String data;
        data.reserve(4096);
        int ET = 1;
        const auto input = socket->getInputStream();
        while (true){
            if(input->available()){
                char buffer[2048] = {0};
                const auto length = input->read(buffer, 2048);
                if(length > 0){
                    ET = 1;
                    data.append(buffer, length);
                }
                else{
                    break;
                }
            }
            else{
                if (ET >= 1000 * 100) {
                    break;
                }
                ET++;
                Thread::SleepMS(1);
            }
        }
        return data;
    }
}

// ========== _doRequest：发送+读取完整响应 ==========
Result HttpClient::_doRequest(ReqConfig& req) {
    if (!_connect()) return Result(-1, "Failed to connect to server");

    const String requestUrl = req.path.empty() ? this->httpUrl : req.path;

    String request = req.method + " " + requestUrl + " HTTP/1.1\r\n";
    if (!req.type.empty()) {
        setHeader("Content-Type", req.type);
        setHeader("Content-Length", std::to_string(req.body.size()));
    }
    for (const auto& header : Headers) {
        request += header.first + ":" + header.second + "\r\n";
    }
    request += "\r\n";

    auto ret = Write(request);
    if (ret.status < 0) { _disconnect(); return ret; }
    if (!req.body.IsEmpty()) {
        ret = Write(req.body);
        if (ret.status < 0) { _disconnect(); return ret; }
    }

    const String response = GetRequestAll();
    _disconnect();

    if (response.empty()) return Result(-1, "Empty response received");

    CCHttpResolver resolver;
    resolver.tryDecodeResponse(response);

    Result result;
    result.status = resolver.getStatusCode();
    result.reason = resolver.getReasonPhrase();
    result.Headers = resolver.getHeaders();
    result.version = resolver.getVersion();
    result.body = resolver.getBody();
    return result;
}

// ========== 快捷请求方法 ==========

Result HttpClient::Get(const std::string& path, const onResCallback& onHeader, const onStreamCallback& onChunk) {
    ReqConfig req;
    req.method = "GET";
    req.path = path;
    if (onChunk) {
        return RequestStream(req, onHeader, onChunk);
    }
    return Request(req);
}

Result HttpClient::Post(const std::string& path, const std::string& body, const std::string& contentType) {
    ReqConfig req;
    req.method = "POST";
    req.path = path;
    req.body = ByteArray::fromString(body);
    req.type = contentType;
    return _doRequest(req);
}

Result HttpClient::Put(const std::string& path, const std::string& body, const std::string& contentType) {
    ReqConfig req;
    req.method = "PUT";
    req.path = path;
    req.body = ByteArray::fromString(body);
    req.type = contentType;
    return _doRequest(req);
}

Result HttpClient::Delete(const std::string& path, const std::string& body) {
    ReqConfig req;
    req.method = "DELETE";
    req.path = path;
    if (!body.empty()) {
        req.body = ByteArray::fromString(body);
        req.type = "application/json";
    }
    return _doRequest(req);
}

Result HttpClient::Patch(const std::string& path, const std::string& body, const std::string& contentType) {
    ReqConfig req;
    req.method = "PATCH";
    req.path = path;
    req.body = ByteArray::fromString(body);
    req.type = contentType;
    return _doRequest(req);
}

Result HttpClient::Head(const std::string& path) {
    ReqConfig req;
    req.method = "HEAD";
    req.path = path;
    return _doRequest(req);
}

Result HttpClient::Options(const std::string& path) {
    ReqConfig req;
    req.method = "OPTIONS";
    req.path = path;
    return _doRequest(req);
}
