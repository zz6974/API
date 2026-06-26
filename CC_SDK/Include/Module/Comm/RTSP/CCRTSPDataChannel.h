#ifndef SRCCTL_RTSP_DATA_CHANNEL_H
#define SRCCTL_RTSP_DATA_CHANNEL_H

#include <chrono>
#include "Socket/CCServerSocket.h"
#include "Socket/CCDatagramSocket.h"
#include "CCSDP.h"

#define CC_R_RTSP_BUFFER_SIZE 4096
#define CC_R_RTSP_HEADER_TIMEOUT_MS 5000
#define CC_R_RTSP_BODY_TIMEOUT_MS 10000

namespace CTL{
    typedef struct RTSP_Message{
        int statusCode = 0;         // 状态码 (响应时使用)
        int port = 0;               // 客户端端口号
        bool isRequest = true;     // 是否为请求消息(true)还是响应消息(false)
        String client_ip;          // 客户端IP
        String url_data;           // 请求URL
        String method;              // RTSP方法 (OPTIONS, DESCRIBE, SETUP, PLAY等)
        String uri;                 // 请求URI
        String uri_path;            // 请求URI路径
        String version;             // RTSP版本 (如"RTSP/1.0")
        String reasonPhrase;        // 原因短语 (响应时使用)
        std::map<String, String> headers;  // 头部字段
        String body;               // 消息体
        String sdp_content;          // SDP内容
        String url_path;            // 请求URL路径 /streamAudio/streamid=0
        String base_path;           // 基础路径 /streamAudio (不包含查询参数)
        std::map<String, String> query_params = {}; // 查询参数字典 {streamid: 0}
    } RTSP_Request;

    class RTSPDataChannel{
        ClientSocket* socket_t = nullptr;
    public:
        explicit RTSPDataChannel(ClientSocket* socket){
            socket_t = socket;
        }
        ~RTSPDataChannel(){
            if (socket_t) {
                socket_t->close();
                socket_t = nullptr;
            }
        }
        static void MessagePrint(const String& message,...){
            System::Println(message);
        }

        bool isConnected() const {
            return socket_t && socket_t->isConnect();
        }

        int receive(char *buffer, const size_t size) const {
            if (socket_t && socket_t->isConnect()) {
                const auto input = socket_t->getInputStream();
                if (input->available()) {
                    return input->read(buffer, size);
                }
                return -2;
            }
            return -1;
        }
        int write(const char *buffer, const size_t size) const {
            if (socket_t && socket_t->isConnect()) {
                const auto output = socket_t->getOutputStream();
                return output->write(buffer,size);
            }
            return 0;
        }

        // 带读超时的 readHeader（生产级：chrono 计时替代忙等计数）
        String readHeader() const{
            return readHeader(CC_R_RTSP_HEADER_TIMEOUT_MS);
        }

        String readHeader(int timeoutMs) const{
            if (!this->socket_t) {
                return "Error_10030";
            }
            String data;
            data.reserve(4096);
            auto startTime = std::chrono::steady_clock::now();
            while (true){
                char buffer[CC_R_RTSP_BUFFER_SIZE] = {0};
                const auto length = receive(buffer, CC_R_RTSP_BUFFER_SIZE);
                if (length > 0) {
                    data.append(buffer, length);
                    const auto pos = data.find("\r\n\r\n");
                    if (pos != CTL::String::npos) {
                        data.resize(pos + 4);
                        return data;
                    }
                    // 防止恶意超长 header（64KB 上限）
                    if (data.length() > 65536) {
                        MessagePrint("RTSPDataChannel readHeader -> Header too large");
                        return "close";
                    }
                }
                else if (length == -2) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - startTime).count();
                    if (elapsed >= timeoutMs) {
                        MessagePrint("RTSPDataChannel readHeader -> Timeout");
                        return "";
                    }
                    Thread::Sleep(1);
                }
                else {
                    return "close";
                }
            }
        }

        String readData(const uint64_t length) const{
            return readData(length, CC_R_RTSP_BODY_TIMEOUT_MS);
        }

        String readData(const uint64_t length, int timeoutMs) const{
            if (!this->socket_t || length == 0) {
                return length == 0 ? "" : "Error_10030";
            }
            String data;
            data.reserve(static_cast<size_t>(length));
            auto startTime = std::chrono::steady_clock::now();
            while (true){
                char buffer[CC_R_RTSP_BUFFER_SIZE] = {0};
                const size_t remaining = static_cast<size_t>(length) - data.length();
                const size_t toRead = remaining < CC_R_RTSP_BUFFER_SIZE ? remaining : CC_R_RTSP_BUFFER_SIZE;
                const auto len = receive(buffer, toRead);
                if (len > 0) {
                    data.append(buffer, len);
                    if (data.length() >= length) {
                        return data;
                    }
                }
                else if (len == -2) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - startTime).count();
                    if (elapsed >= timeoutMs) {
                        MessagePrint("RTSPDataChannel readData -> Timeout");
                        return data;
                    }
                    Thread::Sleep(1);
                }
                else {
                    return "close";
                }
            }
        }
        static RTSP_Message ParseRTSP(const String& data){
            RTSP_Message msg;
            if (data.empty()) {
                return msg;
            }
            msg.url_data = data;
            // 查找空行(\r\n\r\n)来分离头部和消息体
            size_t headerEndPos = data.find("\r\n\r\n");
            String headersPart;
            String bodyPart;
            if (headerEndPos != CTL::String::npos) {
                headersPart = data.substr(0, headerEndPos);
                bodyPart = data.substr(headerEndPos + 4);  // 跳过"\r\n\r\n"
            }
            else {
                headersPart = data;
            }
            // 分割头部行
            std::vector<String> lines;
            String currentLine;
            for (size_t i = 0; i < headersPart.length(); ++i) {
                if (headersPart[i] == '\r' && i + 1 < headersPart.length() && headersPart[i + 1] == '\n') {
                    lines.push_back(currentLine);
                    currentLine.clear();
                    i++; // 跳过'\n'
                } else {
                    currentLine += headersPart[i];
                }
            }
            if (!currentLine.empty()) {
                lines.push_back(currentLine);
            }
            if (!lines.empty()) {
                // 解析第一行 (请求行或状态行)
                const String& firstLine = lines[0];

                // 检查是请求还是响应
                if (firstLine.find("RTSP/") == 0) {
                    // 这是响应行: "RTSP/1.0 200 OK"
                    msg.isRequest = false;

                    size_t firstSpace = firstLine.find(' ');
                    if (firstSpace != String::npos) {
                        size_t secondSpace = firstLine.find(' ', firstSpace + 1);
                        if (secondSpace != String::npos) {
                            msg.version = firstLine.substr(0, firstSpace);
                            try {
                                msg.statusCode = std::stoi(firstLine.substr(firstSpace + 1,
                                    secondSpace - firstSpace - 1));
                            }
                            catch (...) {
                                msg.statusCode = 500;  // 默认错误状态
                            }
                            msg.reasonPhrase = firstLine.substr(secondSpace + 1);
                        }
                    }
                }
                else {
                     // 这是请求行: "OPTIONS rtsp://... RTSP/1.0"
                    size_t firstSpace = firstLine.find(' ');
                    size_t lastSpace = firstLine.rfind(' ');

                    if (firstSpace != String::npos && lastSpace != String::npos && firstSpace < lastSpace) {
                        msg.method = firstLine.substr(0, firstSpace);
                        msg.uri = firstLine.substr(firstSpace + 1, lastSpace - firstSpace - 1);
                        msg.version = firstLine.substr(lastSpace + 1);

                        // 从URI中解析路径部分
                        size_t schemeEnd = msg.uri.find("://");
                        if (schemeEnd != String::npos) {
                            // 跳过协议和主机部分，找到路径
                            size_t hostStart = schemeEnd + 3;
                            size_t pathStart = msg.uri.find('/', hostStart);
                            if (pathStart != String::npos) {
                                msg.uri_path = msg.uri.substr(pathStart);
                            } else {
                                // 如果没有路径，则设置为根路径
                                msg.uri_path = "/";
                            }
                        } else {
                            // 如果没有协议前缀，整个URI可能是路径
                            if (msg.uri.empty() || msg.uri[0] != '/') {
                                msg.uri_path = "/" + msg.uri;
                            } else {
                                msg.uri_path = msg.uri;
                            }
                        }

                        // 解析基础路径和查询参数
                        size_t queryPos = msg.uri_path.find('?');
                        if (queryPos != String::npos) {
                            msg.base_path = msg.uri_path.substr(0, queryPos);
                            String query_string = msg.uri_path.substr(queryPos + 1);

                            // 解析查询参数字符串 (例如: streamid=0&channel=1)
                            size_t start = 0;
                            while (start < query_string.length()) {
                                size_t ampPos = query_string.find('&', start);
                                String param = (ampPos != String::npos) ?
                                    query_string.substr(start, ampPos - start) :
                                    query_string.substr(start);

                                size_t equalPos = param.find('=');
                                if (equalPos != String::npos) {
                                    String key = param.substr(0, equalPos);
                                    String value = param.substr(equalPos + 1);
                                    msg.query_params[key] = value;
                                } else {
                                    msg.query_params[param] = "";
                                }

                                if (ampPos == String::npos) break;
                                start = ampPos + 1;
                            }
                        } else {
                            msg.base_path = msg.uri_path;
                        }
                    }
                }

                // 解析头部字段
                for (size_t i = 1; i < lines.size(); ++i) {
                    const String& line = lines[i];
                    size_t colonPos = line.find(':');
                    if (colonPos != String::npos) {
                        String key = line.substr(0, colonPos);
                        String value = line.substr(colonPos + 1);

                        // 移除键和值前后的空白字符
                        size_t startKey = key.find_first_not_of(" \t");
                        size_t endKey = key.find_last_not_of(" \t");
                        if (startKey != String::npos && endKey != String::npos) {
                            key = key.substr(startKey, endKey - startKey + 1);
                        } else if (startKey == String::npos) {
                            continue;
                        }

                        size_t startPos = value.find_first_not_of(" \t");
                        size_t endPos = value.find_last_not_of(" \t");
                        if (startPos != String::npos && endPos != String::npos) {
                            value = value.substr(startPos, endPos - startPos + 1);
                        } else if (startPos == String::npos) {
                            value.clear();
                        }

                        // 将头部字段转换为小写作为键
                        String lowerKey = key;
                        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

                        msg.headers[lowerKey] = value;

                        // 特殊处理 Transport 头部，提取 client_port 和 mode
                        if (lowerKey == "transport") {
                            // 提取 client_port
                            size_t clientPortPos = value.find("client_port=");
                            if (clientPortPos != String::npos) {
                                clientPortPos += 12;
                                size_t semicolonPos = value.find(';', clientPortPos);
                                String clientPortValue;
                                if (semicolonPos != String::npos) {
                                    clientPortValue = value.substr(clientPortPos, semicolonPos - clientPortPos);
                                } else {
                                    clientPortValue = value.substr(clientPortPos);
                                }
                                msg.headers["client_port"] = clientPortValue;

                                // 分别提取 RTP 和 RTCP 端口
                                size_t dashPos = clientPortValue.find('-');
                                if (dashPos != String::npos) {
                                    msg.headers["client_rtp_port"] = clientPortValue.substr(0, dashPos);
                                    msg.headers["client_rtcp_port"] = clientPortValue.substr(dashPos + 1);
                                }
                            }

                            // 提取 mode
                            size_t modePos = value.find("mode=");
                            if (modePos != String::npos) {
                                modePos += 5;
                                size_t semicolonPos = value.find(';', modePos);
                                String modeValue;
                                if (semicolonPos != String::npos) {
                                    modeValue = value.substr(modePos, semicolonPos - modePos);
                                } else {
                                    modeValue = value.substr(modePos);
                                }
                                msg.headers["mode"] = modeValue;
                            }

                            // 提取 server_port（如果存在）
                            size_t serverPortPos = value.find("server_port=");
                            if (serverPortPos != String::npos) {
                                serverPortPos += 12;
                                size_t semicolonPos = value.find(';', serverPortPos);
                                String serverPortValue;
                                if (semicolonPos != String::npos) {
                                    serverPortValue = value.substr(serverPortPos, semicolonPos - serverPortPos);
                                } else {
                                    serverPortValue = value.substr(serverPortPos);
                                }
                                msg.headers["server_port"] = serverPortValue;
                            }

                            // 提取 unicast/multicast
                            if (value.find("unicast") != String::npos) {
                                msg.headers["transmission_type"] = "unicast";
                            } else if (value.find("multicast") != String::npos) {
                                msg.headers["transmission_type"] = "multicast";
                            }
                        }
                    }
                }
            }
            // 设置消息体
            msg.body = bodyPart;
            return msg;
        }
    };
    class RTSP_Response{
        RTSPDataChannel* socket_t = nullptr;
        std::map<String, String> headers;
        SDPBuilder sdp_builder;
    public:
        //-----------------------------------------------------------------------------------------
        int statusCode = 200;
        String reasonPhrase = "OK";
        bool auto_send = true;
        //-----------------------------------------------------------------------------------------
        void setSocket(RTSPDataChannel* socket){
            this->socket_t = socket;
        }
        RTSPDataChannel* getSocket() const{
            return socket_t;
        }
        String getHeader(const String& key) const{
            auto it = headers.find(key);
            if (it != headers.end()) {
                return it->second;
            }
            return "";
        }
        void setHeader(const String& key, const String& value){
            headers[key] = value;
        }
        String getBuild() const{
            String response = String::format("RTSP/1.0 {} {}\r\n", statusCode, reasonPhrase.c_str());
            for (const auto& header : headers) {
                response.append(String::format("{}: {}\r\n",header.first.c_str(),header.second.c_str()));
            }
            response.append("\r\n");
            return response;
        }
        bool send(){
            if(socket_t != nullptr){
                int ret = socket_t->write(getBuild().c_str(), getBuild().length());
                if (ret > 0) {
                    auto_send = false;
                    return true;
                }
            }
            return false;
        }
        //-----------------------------------------------------------------------------------------
    };
}

#endif