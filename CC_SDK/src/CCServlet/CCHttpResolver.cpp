#include "CCHttpResolver.h"
using namespace CTL;

void CCHttpResolver::tryDecode(const std::string &buf) {
    this->parseInternal(buf.c_str(), buf.size());
}

void HttpResolver::tryDecodeResponse(const std::string& buf) {
    _nextPos = 0;
    _decodeState = HttpRequestDecodeState::START;
    _isChunked = false;
    _body.clear();
    _headers.clear();
    _statusCode = 0;
    _reasonPhrase.clear();
    parseInternal2(buf.c_str(), buf.size());
    if (_isChunked && !_body.empty()) {
        decodeChunkedBody();
    }
}

int HttpResolver::tryDecode2(const std::string& header,const std::vector<char>& buf,
    const std::function<bool(const char*, size_t)>& callback)
{
    // 首次调用或上一轮已完成时：解析 header，重置状态
    if (_stream2State == Stream2State::IDLE || _stream2State == Stream2State::COMPLETE) {
        _stream2Buf.clear();
        _stream2ParsePos = 0;
        _stream2ChunkSize = 0;
        _stream2ContentRemaining = 0;
        _stream2IsChunked = false;
        _headers.clear();
        _statusCode = 0;
        _reasonPhrase.clear();

        const char* p = header.c_str();
        const char* end = p + header.size();

        // 解析状态行: "HTTP/1.1 200 OK"
        const char* protoStart = p;
        while (p < end && *p != ' ' && *p != '\r') ++p;
        _protocol.assign(protoStart, p);
        while (p < end && *p == ' ') ++p;
        const char* codeStart = p;
        while (p < end && isdigit(*p)) ++p;
        _statusCode = atoi(std::string(codeStart, p).c_str());
        while (p < end && *p == ' ') ++p;
        const char* reasonStart = p;
        while (p < end && *p != '\r') ++p;
        _reasonPhrase.assign(reasonStart, p);
        if (p < end && *p == '\r') ++p;
        if (p < end && *p == '\n') ++p;

        // 逐行解析响应头
        while (p < end) {
            const char* keyStart = p;
            while (p < end && *p != ':' && *p != '\r') ++p;
            std::string key(keyStart, p);
            while (p < end && (*p == ':' || *p == ' ')) ++p;
            const char* valStart = p;
            while (p < end && *p != '\r') ++p;
            std::string val(valStart, p);
            _headers[key] = val;
            if (p < end && *p == '\r') ++p;
            if (p < end && *p == '\n') ++p;
        }

        // 判定传输编码类型
        auto te = _headers.find("Transfer-Encoding");
        if (te != _headers.end() && te->second.find("chunked") != std::string::npos) {
            _stream2IsChunked = true;
            _stream2State = Stream2State::CHUNKED_SIZE;
        } else {
            auto cl = _headers.find("Content-Length");
            if (cl != _headers.end()) {
                _stream2ContentRemaining = strtoul(cl->second.c_str(), nullptr, 10);
                _stream2State = _stream2ContentRemaining > 0
                    ? Stream2State::CONTENT_LENGTH
                    : Stream2State::COMPLETE;
            } else {
                _stream2State = Stream2State::RAW;
            }
        }
    }

    // 追加新数据到累积缓冲区
    _stream2Buf.insert(_stream2Buf.end(), buf.begin(), buf.end());

    int chunksProcessed = 0;

    // 结构化模式：chunked / content-length 循环处理
    while (_stream2State == Stream2State::CHUNKED_SIZE
        || _stream2State == Stream2State::CHUNKED_DATA
        || _stream2State == Stream2State::CHUNKED_TRAILER
        || _stream2State == Stream2State::CONTENT_LENGTH) {

        size_t available = _stream2Buf.size() - _stream2ParsePos;
        if (available == 0) break;
        const char* data = _stream2Buf.data() + _stream2ParsePos;

        switch (_stream2State) {
            case Stream2State::CHUNKED_SIZE: {
                const char* crlf = nullptr;
                for (size_t i = 0; i + 1 < available; i++) {
                    if (data[i] == '\r' && data[i + 1] == '\n') {
                        crlf = data + i;
                        break;
                    }
                }
                if (!crlf) return chunksProcessed;

                std::string hexStr(data, crlf - data);
                auto semi = hexStr.find(';');
                if (semi != std::string::npos) hexStr = hexStr.substr(0, semi);
                _stream2ChunkSize = strtoul(hexStr.c_str(), nullptr, 16);
                _stream2ParsePos += (crlf - data) + 2;

                if (_stream2ChunkSize == 0) {
                    _stream2State = Stream2State::CHUNKED_TRAILER;
                } else {
                    _stream2State = Stream2State::CHUNKED_DATA;
                }
                break;
            }

            case Stream2State::CHUNKED_DATA: {
                if (available < _stream2ChunkSize + 2) return chunksProcessed;
                if (!callback(data, _stream2ChunkSize)) {
                    _stream2State = Stream2State::COMPLETE;
                    return chunksProcessed;
                }
                chunksProcessed++;
                _stream2ParsePos += _stream2ChunkSize + 2;
                _stream2State = Stream2State::CHUNKED_SIZE;
                break;
            }

            case Stream2State::CHUNKED_TRAILER: {
                if (available >= 2 && data[0] == '\r' && data[1] == '\n') {
                    _stream2ParsePos += 2;
                    _stream2State = Stream2State::COMPLETE;
                } else {
                    const char* crlf = nullptr;
                    for (size_t i = 0; i + 1 < available; i++) {
                        if (data[i] == '\r' && data[i + 1] == '\n') {
                            crlf = data + i;
                            break;
                        }
                    }
                    if (crlf) {
                        _stream2ParsePos += (crlf - data) + 2;
                    } else {
                        return chunksProcessed;
                    }
                }
                break;
            }

            case Stream2State::CONTENT_LENGTH: {
                size_t consume = available;
                if (consume > _stream2ContentRemaining)
                    consume = _stream2ContentRemaining;
                if (consume == 0) {
                    _stream2State = Stream2State::COMPLETE;
                    break;
                }
                if (!callback(data, consume)) {
                    _stream2State = Stream2State::COMPLETE;
                    return chunksProcessed;
                }
                chunksProcessed++;
                _stream2ParsePos += consume;
                _stream2ContentRemaining -= consume;
                if (_stream2ContentRemaining == 0) {
                    _stream2State = Stream2State::COMPLETE;
                }
                break;
            }

            default:
                break;
        }
    }

    // RAW 模式：无长度标记，收到多少就回调多少
    if (_stream2State == Stream2State::RAW) {
        size_t available = _stream2Buf.size() - _stream2ParsePos;
        if (available > 0) {
            const char* data = _stream2Buf.data() + _stream2ParsePos;
            if (callback(data, available)) {
                chunksProcessed++;
            }
            _stream2ParsePos += available;
        }
    }

    // 定期清理已处理数据，控制内存增长
    if (_stream2ParsePos > 4096) {
        _stream2Buf.erase(_stream2Buf.begin(), _stream2Buf.begin() + _stream2ParsePos);
        _stream2ParsePos = 0;
    }

    return chunksProcessed;
}

int HttpResolver::tryEncrypt(const std::string& header, const std::vector<char>& buf,
    const std::function<bool(const char*, size_t)>& callback)
{
    int chunksProcessed = 0;
    // 首次调用：解析响应头，编码类型，发送 header + \r\n
    if (_encryptState == EncryptState::IDLE) {
        _headers.clear();
        _encryptIsChunked = false;

        const char* p = header.c_str();
        const char* end = p + header.size();

        // 跳过状态行 "HTTP/1.1 200 OK"
        if (header.size() >= 5 && header.substr(0, 5) == "HTTP/") {
            _protocol.assign(p, 8); // "HTTP/1.1"
            while (p < end && *p != ' ') ++p;
            while (p < end && *p == ' ') ++p;
            const char* codeStart = p;
            while (p < end && isdigit(*p)) ++p;
            _statusCode = atoi(std::string(codeStart, p).c_str());
            while (p < end && *p == ' ') ++p;
            const char* reasonStart = p;
            while (p < end && *p != '\r') ++p;
            _reasonPhrase.assign(reasonStart, p);
            if (p < end && *p == '\r') ++p;
            if (p < end && *p == '\n') ++p;
        }

        // 逐行解析响应头
        while (p < end) {
            const char* keyStart = p;
            while (p < end && *p != ':' && *p != '\r') ++p;
            std::string key(keyStart, p);
            while (p < end && (*p == ':' || *p == ' ')) ++p;
            const char* valStart = p;
            while (p < end && *p != '\r') ++p;
            std::string val(valStart, p);
            _headers[key] = val;
            if (p < end && *p == '\r') ++p;
            if (p < end && *p == '\n') ++p;
        }

        // 检测 chunked 编码
        auto te = _headers.find("Transfer-Encoding");
        if (te != _headers.end() && te->second.find("chunked") != std::string::npos) {
            _encryptIsChunked = true;
        }

        // 发送响应头 + 空行（头与 body 的分隔）
        std::string headerBlock = header + "\r\n";
        if (!callback(headerBlock.c_str(), headerBlock.size())) {
            _encryptState = EncryptState::COMPLETE;
            return 0;
        }
        chunksProcessed++;

        _encryptState = EncryptState::SENDING_BODY;
    }

    if (_encryptState != EncryptState::SENDING_BODY) return chunksProcessed;

    // 空 buf = body 结束信号
    if (buf.empty()) {
        if (_encryptIsChunked) {
            const char* term = "0\r\n\r\n";
            bool f = callback(term, 5);
            if (!f) {
                return -1;
            }
            chunksProcessed++;
        }
        _encryptState = EncryptState::COMPLETE;
        return chunksProcessed;
    }

    // 编码并发送 body 数据
    if (_encryptIsChunked) {
        // Chunked: <hex size>\r\n<data>\r\n
        char hexBuf[32];
        snprintf(hexBuf, sizeof(hexBuf), "%zx\r\n", buf.size());
        std::string hexStr(hexBuf);

        std::vector<char> encoded;
        encoded.reserve(hexStr.size() + buf.size() + 2);
        encoded.insert(encoded.end(), hexStr.begin(), hexStr.end());
        encoded.insert(encoded.end(), buf.begin(), buf.end());
        encoded.push_back('\r');
        encoded.push_back('\n');

        if (!callback(encoded.data(), encoded.size())) {
            _encryptState = EncryptState::COMPLETE;
            return chunksProcessed;
        }
        chunksProcessed++;
    }
    else {
        // Content-Length / 无标记：直接透传
        if (!callback(buf.data(), buf.size())) {
            _encryptState = EncryptState::COMPLETE;
            return chunksProcessed;
        }
        chunksProcessed++;
    }

    return chunksProcessed;
}

void CCHttpResolver::parseInternal(const char *buf, const int size) {

    StringBuffer method;
    StringBuffer url;

    StringBuffer requestParamKey;
    StringBuffer requestParamValue;

    StringBuffer protocol;
    StringBuffer version;

    StringBuffer headerKey;
    StringBuffer headerValue;

    int bodyLength = 0;

    char *p0 = const_cast<char *>(buf + _nextPos);//去掉const限制

    while (_decodeState != HttpRequestDecodeState::INVALID
           && _decodeState != HttpRequestDecodeState::INVALID_METHOD
           && _decodeState != HttpRequestDecodeState::INVALID_URI
           && _decodeState != HttpRequestDecodeState::INVALID_VERSION
           && _decodeState != HttpRequestDecodeState::INVALID_HEADER
           && _decodeState != HttpRequestDecodeState::COMPLETE
           && _nextPos < size) {

        char ch = *p0;//当前的字符
        char *p = p0++;//指针偏移
        int scanPos = _nextPos++;//下一个指针往后偏移

        switch (_decodeState) {
            case HttpRequestDecodeState::START: {
                //空格 换行 回车都继续
                if (ch == CR || ch == LF || isblank(ch)) {
                    //do nothing
                } else if (isupper(ch)) {//判断是不是大写字符，不是的话是无效的
                    method.begin = p;//方法的起始点
                    _decodeState = HttpRequestDecodeState::METHOD;//如果遇到第一个字符，开始解析方法
                } else {
                    _decodeState = HttpRequestDecodeState::INVALID;
                }
                break;
            }
            case HttpRequestDecodeState::METHOD: {
                //方法需要大写字母，大写字母就继续
                if (isupper(ch)) {
                    //do nothing
                } else if (isblank(ch)) {//空格，说明方法解析结束，下一步开始解析URI
                    method.end = p;//方法解析结束
                    _method = method;
                    _decodeState = HttpRequestDecodeState::BEFORE_URI;
                } else {
                    _decodeState = HttpRequestDecodeState::INVALID_METHOD;//其他情况是无效的请求方法
                }
                break;
            }
            case HttpRequestDecodeState::BEFORE_URI: {
                //请求连接前的处理，需要'/'开头
                if (ch == '/') {
                    url.begin = p;
                    _decodeState = HttpRequestDecodeState::IN_URI;//下一步就是开始处理连接
                } else if (isblank(ch)) {
                    //do nothing
                } else {
                    _decodeState = HttpRequestDecodeState::INVALID;//无效的
                }
                break;
            }
            case HttpRequestDecodeState::IN_URI: {
                //开始处理请求路径的字符串
                if (ch == '?') {//转为处理请求的key值
                    url.end = p;
                    _url = url;
                    _decodeState = HttpRequestDecodeState::BEFORE_URI_PARAM_KEY;
                } else if (isblank(ch)) {//遇到空格，请求路径解析完成，开始解析协议
                    url.end = p;
                    _url = url;
                    _decodeState = HttpRequestDecodeState::BEFORE_PROTOCOL;
                } else {
                    //do nothing
                }
                break;
            }
            case HttpRequestDecodeState::BEFORE_URI_PARAM_KEY: {
                if (isblank(ch) || ch == LF || ch == CR) {
                    _decodeState = HttpRequestDecodeState::INVALID_URI;
                } else {
                    requestParamKey.begin = p;
                    _decodeState = HttpRequestDecodeState::URI_PARAM_KEY;
                }
                break;
            }
            case HttpRequestDecodeState::URI_PARAM_KEY: {
                if (ch == '=') {
                    requestParamKey.end = p;
                    _decodeState = HttpRequestDecodeState::BEFORE_URI_PARAM_VALUE;//开始解析参数值
                } else if (isblank(ch)) {
                    _decodeState = HttpRequestDecodeState::INVALID_URI;//无效的请求参数
                } else {
                    //do nothing
                }
                break;
            }
            case HttpRequestDecodeState::BEFORE_URI_PARAM_VALUE: {
                if (isblank(ch) || ch == LF || ch == CR) {
                    _decodeState = HttpRequestDecodeState::INVALID_URI;
                } else {
                    requestParamValue.begin = p;
                    _decodeState = HttpRequestDecodeState::URI_PARAM_VALUE;
                }
                break;
            }
            case HttpRequestDecodeState::URI_PARAM_VALUE: {
                if (ch == '&') {
                    requestParamValue.end = p;
                    //收获一个请求参数
                    _requestParams.insert({requestParamKey, requestParamValue});
                    _decodeState = HttpRequestDecodeState::BEFORE_URI_PARAM_KEY;
                } else if (isblank(ch)) {
                    requestParamValue.end = p;
                    //空格也收获一个请求参数
                    _requestParams.insert({requestParamKey, requestParamValue});
                    _decodeState = HttpRequestDecodeState::BEFORE_PROTOCOL;
                } else {
                    //do nothing
                }
                break;
            }
            case HttpRequestDecodeState::BEFORE_PROTOCOL: {
                if (isblank(ch)) {
                    //do nothing
                } else {
                    protocol.begin = p;
                    _decodeState = HttpRequestDecodeState::PROTOCOL;
                }
                break;
            }
            case HttpRequestDecodeState::PROTOCOL: {
                //解析请求协议
                if (ch == '/') {
                    protocol.end = p;
                    _protocol = protocol;
                    _decodeState = HttpRequestDecodeState::BEFORE_VERSION;
                } else {
                    //do nothing
                }
                break;
            }
            case HttpRequestDecodeState::BEFORE_VERSION: {
                if (isdigit(ch)) {
                    version.begin = p;
                    _decodeState = HttpRequestDecodeState::VERSION;
                } else {
                    _decodeState = HttpRequestDecodeState::INVALID_VERSION;
                }
                break;
            }
            case HttpRequestDecodeState::VERSION: {
                //协议解析，如果不是数字或者. 就不对
                if (ch == CR) {
                    version.end = p;
                    _version = version;
                    _decodeState = HttpRequestDecodeState::WHEN_CR;
                } else if (ch == '.') {
                    //遇到版本分割
                    _decodeState = HttpRequestDecodeState::VERSION_SPLIT;
                } else if (isdigit(ch)) {
                    //do nothing
                } else {
                    _decodeState = HttpRequestDecodeState::INVALID_VERSION;//不能不是数字
                }
                break;
            }
            case HttpRequestDecodeState::VERSION_SPLIT: {
                //遇到版本分割符，字符必须是数字，其他情况都是错误
                if (isdigit(ch)) {
                    _decodeState = HttpRequestDecodeState::VERSION;
                } else {
                    _decodeState = HttpRequestDecodeState::INVALID_VERSION;
                }
                break;
            }
            case HttpRequestDecodeState::HEADER_KEY: {
                //冒号前后可能有空格
                if (isblank(ch)) {
                    headerKey.end = p;
                    _decodeState = HttpRequestDecodeState::HEADER_BEFORE_COLON;//冒号之前的状态
                } else if (ch == ':') {
                    headerKey.end = p;
                    _decodeState = HttpRequestDecodeState::HEADER_AFTER_COLON;//冒号之后的状态
                } else {
                    //do nothing
                }
                break;
            }
            case HttpRequestDecodeState::HEADER_BEFORE_COLON: {
                if (isblank(ch)) {
                    //do nothing
                } else if (ch == ':') {
                    _decodeState = HttpRequestDecodeState::HEADER_AFTER_COLON;
                } else {
                    //冒号之前的状态不能是空格之外的其他字符
                    _decodeState = HttpRequestDecodeState::INVALID_HEADER;
                }
                break;
            }
            case HttpRequestDecodeState::HEADER_AFTER_COLON: {
                if (isblank(ch)) {//值之前遇到空格都是正常的
                    //do nothing
                } else {
                    headerValue.begin = p;
                    _decodeState = HttpRequestDecodeState::HEADER_VALUE;//开始处理值
                }
                break;
            }
            case HttpRequestDecodeState::HEADER_VALUE: {
                if (ch == CR) {
                    headerValue.end = p;
                    _headers.insert({headerKey, headerValue});
                    _decodeState = HttpRequestDecodeState::WHEN_CR;
                }
                break;
            }
            case HttpRequestDecodeState::WHEN_CR: {
                if (ch == LF) {
                    //如果是回车，可换成下一个
                    _decodeState = HttpRequestDecodeState::CR_LF;
                } else {
                    _decodeState = HttpRequestDecodeState::INVALID;
                }
                break;
            }
            case HttpRequestDecodeState::CR_LF: {
                if (ch == CR) {
                    //如果在CR_LF状态之后还有CR，那么便是有点结束的味道了
                    _decodeState = HttpRequestDecodeState::CR_LF_CR;
                } else if (isblank(ch)) {
                    _decodeState = HttpRequestDecodeState::INVALID;
                } else {
                    //如果不是，那么就可能又是一个请求头了，那就开始解析请求头
                    headerKey.begin = p;
                    _decodeState = HttpRequestDecodeState::HEADER_KEY;
                }
                break;
            }
            case HttpRequestDecodeState::CR_LF_CR: {
                if (ch == LF) {
                    //如果是\r接着\n 那么判断是不是需要解析请求体
                    auto te = _headers.find("Transfer-Encoding");
                    if (te != _headers.end() && te->second.find("chunked") != std::string::npos) {
                        _isChunked = true;
                    }
                    if (_headers.count("Content-Length") > 0) {
                        bodyLength = atoi(_headers["Content-Length"].c_str());
                        if (bodyLength > 0) {
                            _decodeState = HttpRequestDecodeState::BODY;//解析请求体
                        } else {
                            _decodeState = HttpRequestDecodeState::COMPLETE;//完成了
                        }
                    } else {
                        if (scanPos < size) {
                            bodyLength = size - scanPos;
                            _decodeState = HttpRequestDecodeState::BODY;//解析请求体
                        } else {
                            _decodeState = HttpRequestDecodeState::COMPLETE;
                        }
                    }
                } else {
                    _decodeState = HttpRequestDecodeState::INVALID_HEADER;
                }
                break;
            }
            case HttpRequestDecodeState::BODY: {
                //解析请求体
                _body.assign(p, bodyLength);
                _decodeState = HttpRequestDecodeState::COMPLETE;
                break;
            }
            default:
                break;
        }
    }
}

void HttpResolver::parseInternal2(const char* buf,const int size) {
    const char* p = buf;
    const char* end = buf + size;

    // 解析协议: "HTTP/1.1"
    while (p < end && *p != ' ' && *p != '\r' && *p != '\n') ++p;
    _protocol.assign(buf, p);

    // 跳过空格
    while (p < end && *p == ' ') ++p;

    // 解析状态码
    const char* codeStart = p;
    while (p < end && isdigit(*p)) ++p;
    _statusCode = atoi(std::string(codeStart, p).c_str());

    // 跳过空格
    while (p < end && *p == ' ') ++p;

    // 解析原因短语
    const char* reasonStart = p;
    while (p < end && *p != '\r' && *p != '\n') ++p;
    _reasonPhrase.assign(reasonStart, p);

    // 跳过 \r\n
    if (p < end && *p == '\r') ++p;
    if (p < end && *p == '\n') ++p;

    _nextPos = p - buf;

    if (_nextPos >= size) {
        _decodeState = HttpRequestDecodeState::COMPLETE;
        return;
    }

    // 复用现有状态机解析 header + body
    _decodeState = HttpRequestDecodeState::CR_LF;
    parseInternal(buf, size);
}

void HttpResolver::decodeChunkedBody() {
    std::string decoded;
    const char* p = _body.c_str();
    const char* end = p + _body.size();

    while (p < end) {
        char* hexEnd = nullptr;
        long chunkSize = strtol(p, &hexEnd, 16);
        if (chunkSize <= 0) break;

        p = hexEnd;
        while (p < end && (*p == '\r' || *p == '\n')) ++p;

        if (p + chunkSize > end) break;
        decoded.append(p, chunkSize);
        p += chunkSize;
        while (p < end && (*p == '\r' || *p == '\n')) ++p;
    }

    _body = std::move(decoded);
}

const std::string &CCHttpResolver::getMethod() const {
    return _method;
}

const std::string &CCHttpResolver::getUrl() const {
    return _url;
}

const std::map<std::string, std::string> &CCHttpResolver::getRequestParams() const {
    return _requestParams;
}

const std::string &CCHttpResolver::getProtocol() const {
    return _protocol;
}

const std::string &CCHttpResolver::getVersion() const {
    return _version;
}

const std::map<CTL::String, CTL::String> &CCHttpResolver::getHeaders() const {
    return _headers;
}

const std::string &CCHttpResolver::getBody() const {
    return _body;
}

int HttpResolver::getStatusCode() const {
    return _statusCode;
}

const std::string& HttpResolver::getReasonPhrase() const {
    return _reasonPhrase;
}

bool HttpResolver::isStream2Complete() const {
    return _stream2State == Stream2State::COMPLETE;
}

size_t HttpResolver::getStream2Remaining() const {
    return _stream2ContentRemaining;
}
