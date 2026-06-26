#include "WebSocket/CCWebSocket.h"

#include <openssl/err.h>
#include <openssl/ssl.h>
#include "OpenSSL/CCSSL.h"

// extern "C"{
// #include "openssl/applink.h"
// }

using namespace CTL;

WebSocketPacket& CTL::WebSocketInfo::GetPacket() {
    return m_packet;
}

CTL::Socket& CTL::WebSocketInfo::GetSock() {
    return m_sock;
}

const WebSocketPacket& CTL::WebSocketInfo::GetPacket() const {
    return m_packet;
}

void CTL::WebSocketInfo::SetPacket(const WebSocketPacket& packet) {
    m_packet = packet;
}

void CTL::WebSocketInfo::SetSock(const CTL::Socket& socket) {
    m_sock = socket;
}

void CTL::WebSocketInfo::SetBuffer(ByteBuffer& buffer) {
    m_buffer.assign(buffer.bytes(), static_cast<unsigned int>(buffer.length()));
}

ByteArray CTL::WebSocketInfo::GetBuffer() const{
    return m_buffer;
}

bool CTL::WebSocketInfo::GetConnectFlag() const {
    return Con_Flag;
}

void CTL::WebSocketInfo::SetConnectFlag(const bool F) {
    Con_Flag = F;
}

void CTL::WebSocketInfo::SetID(const SOCKET ID) {
    this->ID = ID;
}

SOCKET CTL::WebSocketInfo::GetID() const {
    return ID;
}

int CTL::WebSocketInfo::WriteSSLResponse(SSL* ssl, const char* buffer, const size_t size) {
    if (ssl == nullptr) {
        return -1; // 错误ssl
    }

#if __linux__
    // 在Linux系统中，防止SSL_write触发SIGPIPE信号
    // 该设置已在InitSSL中通过sigaction处理，此处作为额外保障
    // struct sigaction sa;
    // sa.sa_handler = SIG_IGN;
    // sigaction(SIGPIPE, &sa, nullptr);
    signal(SIGPIPE, SIG_IGN);
#endif

    const int result = SSL_write(ssl, buffer, static_cast<int>(size));
    if (result <= 0) {
        const int err = SSL_get_error(ssl, result);
        switch (err) {
            case SSL_ERROR_ZERO_RETURN: {
                // 对方已关闭连接，这是正常情况
                return 0;
            }
            case SSL_ERROR_WANT_READ: {
                // 非阻塞操作，需要等待可读事件
                return -5;
            }
            case SSL_ERROR_WANT_WRITE: {
                // 非阻塞操作，需要等待可写事件
                return -5;
            }
            case SSL_ERROR_SYSCALL: {
                // 系统调用错误，可能包括SIGPIPE
                if (ERR_peek_error() == 0) {
                    // 如果没有SSL错误，检查系统错误
                    if (errno == EPIPE || errno == ECONNRESET) {
                        // 连接已断开或管道破裂
                        return -4;
                    }
                }
                // 其他系统错误
                return -3;
            }
            case SSL_ERROR_SSL: {
                // SSL协议错误
                ERR_print_errors_fp(stderr);
                return -2;
            }
            default: {
                // 其他SSL错误
                return 99;
            }
        }
    }

    // 检查写入字节数是否匹配期望值
    if (static_cast<size_t>(result) != size) {
        // 写入不完整，可能需要重试或处理错误
        return 98;
    }

    return 1;
}

bool WebSocketClient::Send_Handshake(const String &ip_, const int port_,const String& Path, const String &ssh) const {
    const std::string handshake = "GET "+Path+" HTTP/1.1\r\n"
                                "Host: " + ip_ + ":" + std::to_string(port_) + "\r\n"
                                "Upgrade: websocket\r\n"
                                "Connection: Upgrade\r\n"
                                "Sec-WebSocket-Key: "+ ssh +"\r\n"
                                "Sec-WebSocket-Protocol: chat, superchat\r\n"
                                "Sec-WebSocket-Extensions: CC_SDK QingJiao\r\n"
                                "Sec-WebSocket-Version: 13\r\n\r\n";

    if (send(m_sock.Socketbit, handshake.c_str(), handshake.size(), 0) != handshake.size()) {
        perror("send");
        return false;
    }
    return true;
}

bool WebSocketClient::Recv_Handshake() {
    int ET = 0;
    while (true) {
        if (m_sock.isDataAvailable()) {
            char buffer[2048] = {0};
            const size_t bytes_received = m_sock.RecvData(buffer,sizeof buffer);
            if (bytes_received <= 0) {
                CC::TimePrintln("Recv_Handshake -> 连接失败!");
                return false;
            }
            buffer[bytes_received] = '\0';
            std::string response(buffer);

            // 提取 Sec-WebSocket-Accept 字段
            std::string sec_websocket_accept;
            size_t pos = response.find("Sec-WebSocket-Accept:");
            if (pos != std::string::npos) {
                pos += strlen("Sec-WebSocket-Accept:");
                const size_t end_pos = response.find("\r\n", pos);
                if (end_pos != std::string::npos) {
                    sec_websocket_accept = response.substr(pos, end_pos - pos);
                    WebSocket::trim(sec_websocket_accept); // 去除首尾空格
                }
            }

            // 验证 Sec-WebSocket-Accept
            if (!WebSocketPacket::ValidateSecWebSocketAccept(ClientKey, sec_websocket_accept)) {
                std::cerr << "Invalid Sec-WebSocket-Accept" << std::endl;
                return false;
            }

            // 简单检查握手响应是否成功
            if (response.find("101 Switching Protocols") == std::string::npos &&
            response.find("101 Web Socket Protocol Handshake") == std::string::npos) {
                std::cerr << "Handshake failed: " << response << std::endl;
                return false;
            }
            break;
        }
        else {
            if (ET >= 2000) {
                CC::TimePrintln("Recv_Handshake -> 连接超时!");
                return false;
            }
            Thread::Sleep(1000);
            ET++;
        }
    }
    return true;
}

std::string WebSocket::generate_sec_websocket_key() {
    unsigned char random_bytes[16] = {0};

    // 生成随机字节
    if (RAND_bytes(random_bytes, sizeof(random_bytes)) != 1) {
        throw std::runtime_error("Failed to generate random bytes");
    }

    // 使用 EVP_EncodeBlock 进行 Base64 编码
    unsigned char base64_encoded[25]; // 16 字节的 Base64 编码后最多为 24 字节 + 1 个终止符
    int encoded_len = EVP_EncodeBlock(base64_encoded, random_bytes, sizeof(random_bytes));
    if (encoded_len <= 0) {
        throw std::runtime_error("Failed to encode Base64");
    }

    // 将结果转换为 std::string 并去除末尾的换行符
    std::string base64_key(reinterpret_cast<char*>(base64_encoded), encoded_len);
    base64_key.erase(std::remove(base64_key.begin(), base64_key.end(), '\n'), base64_key.end());

    return base64_key;
}

WebSocketClient::WebSocketClient_Info WebSocketClient::ParsingLink(const std::string& url) {
    WebSocketClient_Info ws;
    // 找到协议部分的结束位置
    const size_t protocolEnd = url.find("://");
    if (protocolEnd == std::string::npos) {
        std::cerr << "Invalid URL format" << std::endl;
        return {};
    }
    // 提取协议部分（虽然在这个例子中不需要，但为了完整性）
    const std::string protocol = url.substr(0, protocolEnd);
    // 找到主机部分的结束位置
    size_t hostEnd = url.find('/', protocolEnd + 3);
    if (hostEnd == std::string::npos) {
        hostEnd = url.length();
    }
    // 提取主机部分
    std::string host = url.substr(protocolEnd + 3, hostEnd - (protocolEnd + 3));
    // 在主机部分中找到端口号的开始位置
    size_t portStart = host.find(':');
    std::string ip;
    std::string port;
    if (portStart != std::string::npos) {
        // 提取 IP 地址
        ip = host.substr(0, portStart);
        // 提取端口号
        port = host.substr(portStart + 1);
    } else {
        // 如果没有找到端口号，假设默认端口
        ip = host;
        port = ""; // 或者设置为默认端口，例如 "80" 或 "443"
    }
    // 提取路由部分
    std::string path;
    if (hostEnd != url.length()) {
        path = url.substr(hostEnd);
    }
    ws.Protocol = protocol;
    ws.IP = ip;
    ws.Prot = std::stoi(port);
    ws.Path = path;
    return ws;
}

bool CTL::WebSocket::Init(const CTL::String& ip, const int port, const bool ChokeUp) {
    m_sock.Init(IPVX::IPV4, TORU::TCP);
    const bool F = m_sock.Bind(ip.c_str(), port);
    if (!ChokeUp) {
        m_sock.SetSocketNonBlocking();
    }
    return F;
}

bool CTL::WebSocket::Init(const WebSocketBind& bind) {
    this->OnBind(bind.URL, bind.OnOpenFun, bind.OnCloseFun, bind.OnMessageFun,
        bind.OnErrorFun, bind.OnPingFun, bind.OnPongFun);
    return true;
}

void WebSocket::SetSSL(SSL_CTX *ssl_ctx_t) {
    this->ssl_ctx = ssl_ctx_t;
}

bool WebSocketClient::Connect(const CTL::String &ip, const int port,const String& URL) {
    Client = true;
    ServerFlag = true;
    ClientOK = false;
    m_sock.Init(IPVX::IPV4, TORU::TCP);
    LocalClientInfo.SetConnectFlag(true);
    if (bool F = m_sock.Connect(ip.c_str(), port)) {
        if (this->ssl_ctx) {
            // 创建SSL对象
            SSL* ssl = SSL_new(ssl_ctx);
            if (!ssl) {
                ERR_print_errors_fp(stderr);
                return false;
            }
            // 将SSL对象与Socket关联
            SSL_set_fd(ssl, m_sock.Socketbit);
            // 进行SSL握手
            if (SSL_connect(ssl) <= 0) {
                ERR_print_errors_fp(stderr);
                SSL_free(ssl);
                return false;
            }
            LocalClientInfo.ssl = ssl;
        }
        ClientKey = WebSocket::generate_sec_websocket_key();
        F = Send_Handshake(ip, port,URL,ClientKey);
        if (F) {
            ClientOK = Recv_Handshake();
        }
    }
    return ClientOK;
}

bool WebSocketClient::Connect(const CTL::String &WsURL) {
    const auto ws = ParsingLink(WsURL);
    return Connect(ws.IP.c_str(), ws.Prot, ws.Path);
}

int WebSocketClient::Running() {
    WebSocketPacket m_packet;
    LocalClientInfo.SetConnectFlag(true);
    LocalClientInfo.SetSock(m_sock);
    try {
        if (OnOpenFun) {
            LocalClientInfo.SetPacket(m_packet);
            OnOpenFun(LocalClientInfo);
        }
    }
    catch (CCException& exception) {
        CC::TimePrintln("OnOpen -> " + CTL::String(exception.what()));
    }
    bool is_client_error = false;
    while (ServerFlag && ClientOK && LocalClientInfo.GetConnectFlag()) {
        if (m_sock.isDataAvailable()) {
            char buf[1024] = {0};
            const auto length = WebSocket::ReadBuffer(LocalClientInfo.ssl,m_sock,buf,sizeof buf);
            if (length > 0 && !is_client_error) {
                if (length == -1) {
                    is_client_error = true;
                    continue;
                }
                ClientBuffer.append(buf,length);
                const uint64_t frameSize = m_packet.recv_dataframe(ClientBuffer);
                if (frameSize > 0) {
                    // 成功接收到一个完整的帧
                    const uint8_t opcode = m_packet.get_opcode();
                    if (opcode == WebSocketPacket::WSOpcode_Close) {
                        if (OnCloseFun) {
                            try {
                                LocalClientInfo.SetPacket(m_packet);
                                OnCloseFun(LocalClientInfo);
                            }
                            catch (CCException& exception) {
                                CC::TimePrintln(exception.what());
                            }
                        }
                        if (LocalClientInfo.GetConnectFlag()) {
                            // 构建关闭帧
                            char closeFrame[2 + 125]; // 最大关闭帧大小为125字节
                            closeFrame[0] = 0b10001000; // FIN=1, Opcode=0x8
                            closeFrame[1] = 2; // 负载长度为2，表示只有状态码
                            closeFrame[2] = 0x03; // 状态码高字节，1000表示正常关闭
                            closeFrame[3] = 0xE8; // 状态码低字节
                            // 发送关闭帧
                            LocalClientInfo.Write(closeFrame, 4);
                        }
                        break;
                    }
                    else if (opcode == WebSocketPacket::WSOpcode_Binary || opcode == WebSocketPacket::WSOpcode_Text) {
                        try {
                            LocalClientInfo.SetPacket(m_packet);
                            // 获取payload数据
                            ByteBuffer payload = m_packet.get_payload();
                            LocalClientInfo.SetBuffer(payload);
                            OnMessageFun(LocalClientInfo);
                        }
                        catch (CCException& exception) {
                            CC::TimePrintln(exception.what());
                        }
                    }
                    else if (opcode == WebSocketPacket::WSOpcode_Ping) {
                        LocalClientInfo.SetPacket(m_packet);
                        if (OnPingFun) {
                            OnPingFun(LocalClientInfo);
                        }
                        else {
                            // 构建Pong消息
                            char pongFrame[2 + 125]; // 最大Pong帧大小为125字节
                            pongFrame[0] = 0b10001010; // FIN=1, Opcode=0xA (Pong)
                            pongFrame[1] = 0; // 负载长度为0
                            // 发送Pong消息
                            LocalClientInfo.Write(pongFrame, 2);
                        }
                    }
                    else if (opcode == WebSocketPacket::WSOpcode_Pong) {
                        LocalClientInfo.SetPacket(m_packet);
                        if (OnPongFun) {
                            OnPongFun(LocalClientInfo);
                        }
                    }
                    ClientBuffer.clear();
                    m_packet.payload_.clear();
                }
            }
            else {
                try {
                    if (OnErrorFun) {
                        LocalClientInfo.SetPacket(m_packet);
                        OnErrorFun(LocalClientInfo);
                    }
                }
                catch (CCException& e) {
                    CC::Println("ERFun -> " + CTL::String(e.what()));
                }
                break;
            }
        }
        else {
            Thread::Sleep(1000);
        }
    }
    return 0;
}

void WebSocketClient::Stop() {
    ServerFlag = false;
    m_sock.Close();
    LocalClientInfo.CloseConnection(1000,"Client Close");
}

bool WebSocketClient::SendBinary(const ByteArray& data) {
    return LocalClientInfo.SendBinary(data);
}

bool WebSocketClient::SendText(const String& text) {
    return LocalClientInfo.SendText(text);
}

int CTL::WebSocket::Running() {
    const bool F = m_sock.Listen();
    if (!F) {
        return -1;
    }
    M_S_POOl.InitStart(M_Pool_Size, M_Pool_Max, M_Pool_KeepAliveTime);
    InitService();
    event_loop_t.registerEvent(m_sock.Socketbit,[this](uint8_t event_type) {
        AcceptingClient();
    });
    event_loop_t.run();
    Stop();
    return 0;
}

void CTL::WebSocket::OnBind(const CTL::String& URL, const RequestFunc& OnOpenFun, const RequestFunc& OnCloseFun,
    const RequestFunc& OnMessageFun, const RequestFunc& OnErrorFun,const RequestFunc& OnPingFun ,const RequestFunc& OnPongFun) {
    URLS[URL] = true;
    URLOpenFun.insert(std::pair<CTL::String, RequestFunc>(URL, OnOpenFun));
    URLCloseFun.insert(std::pair<CTL::String, RequestFunc>(URL, OnCloseFun));
    URLMessageFun.insert(std::pair<CTL::String, RequestFunc>(URL, OnMessageFun));
    URL_ERROR_Fun.insert(std::pair<CTL::String, RequestFunc>(URL, OnErrorFun));
    URL_Ping_Fun.insert(std::pair<CTL::String, RequestFunc>(URL, OnPingFun));
    URL_Pong_Fun.insert(std::pair<CTL::String, RequestFunc>(URL, OnPongFun));
}

void WebSocketClient::OnBind(const RequestFunc &OnOpenFun, const RequestFunc &OnCloseFun, const RequestFunc &OnMessageFun,
    const RequestFunc &OnErrorFun,const RequestFunc& OnPingFun,const RequestFunc& OnPongFun) {
    this->OnOpenFun = OnOpenFun;
    this->OnCloseFun = OnCloseFun;
    this->OnMessageFun = OnMessageFun;
    this->OnErrorFun = OnErrorFun;
    this->OnPingFun = OnPingFun;
    this->OnPongFun = OnPongFun;
}

bool CTL::WebSocketInfo::Write(const char* data, const size_t size) {
    std::lock_guard<std::mutex> lock(m_mutex_id);
    if (this->ssl) {
        m_status = true;
        const int ret = WriteSSLResponse(this->ssl, data, size);
        if (ret == 1) {
            m_status = false;
            return true;
        }
        else {
            if (ret == 0 || ret == -4 || ret == -2 || ret == -3 || ret == 99) {
                auto sock = this->GetSock();
                sock.Close();
            }
            m_status = false;
            return false;
        }
    }
    else {
        return this->m_sock.Send(data, size, MSL_N);
    }
}

bool CTL::WebSocketInfo::SendText(const CTL::String& text) {
    const size_t payloadLength = text.size();
    char* frame = nullptr;
    size_t frameSize = 0;

    if (payloadLength <= 125) {
        frameSize = payloadLength + 2;
        frame = new char[frameSize];
        frame[0] = 0b10000001; // FIN=1, Opcode=0x1 (Text frame)
        frame[1] = static_cast<char>(payloadLength);
        memcpy(frame + 2, text.c_str(), payloadLength);
    } else if (payloadLength <= 65535) {
        frameSize = payloadLength + 4;
        frame = new char[frameSize];
        frame[0] = 0b10000001; // FIN=1, Opcode=0x1 (Text frame)
        frame[1] = 126; // 126 indicates that the next 2 bytes are the payload length
        frame[2] = static_cast<char>((payloadLength >> 8) & 0xFF);
        frame[3] = static_cast<char>(payloadLength & 0xFF);
        memcpy(frame + 4, text.c_str(), payloadLength);
    } else {
        frameSize = payloadLength + 10;
        frame = new char[frameSize];
        frame[0] = 0b10000001; // FIN=1, Opcode=0x1 (Text frame)
        frame[1] = 127; // 127 indicates that the next 8 bytes are the payload length
        for (int i = 0; i < 8; ++i) {
            frame[2 + i] = static_cast<char>((payloadLength >> (56 - 8 * i)) & 0xFF);
        }
        memcpy(frame + 10, text.c_str(), payloadLength);
    }

    const bool F = this->Write(frame, frameSize);
    delete[] frame;
    return F;
}

bool CTL::WebSocketInfo::SendBinary(const ByteArray& binaryData) {
    const size_t payloadLength = binaryData.size();
    char* frame = nullptr;
    size_t frameSize = 0;

    if (payloadLength <= 125) {
        frameSize = payloadLength + 2;
        frame = new char[frameSize];
        frame[0] = 0b10000010; // FIN=1, Opcode=0x2 (Binary frame)
        frame[1] = static_cast<char>(payloadLength);
        memcpy(frame + 2, binaryData.buffer(), payloadLength);
    } else if (payloadLength <= 65535) {
        frameSize = payloadLength + 4;
        frame = new char[frameSize];
        frame[0] = 0b10000010; // FIN=1, Opcode=0x2 (Binary frame)
        frame[1] = 126; // 126 indicates that the next 2 bytes are the payload length
        frame[2] = static_cast<char>((payloadLength >> 8) & 0xFF);
        frame[3] = static_cast<char>(payloadLength & 0xFF);
        memcpy(frame + 4, binaryData.buffer(), payloadLength);
    } else {
        frameSize = payloadLength + 10;
        frame = new char[frameSize];
        frame[0] = 0b10000010; // FIN=1, Opcode=0x2 (Binary frame)
        frame[1] = 127; // 127 indicates that the next 8 bytes are the payload length
        for (int i = 0; i < 8; ++i) {
            frame[2 + i] = static_cast<char>((payloadLength >> (56 - 8 * i)) & 0xFF);
        }
        memcpy(frame + 10, binaryData.buffer(), payloadLength);
    }

    const bool F = this->Write(frame, frameSize);
    delete[] frame;
    return F;
}

void CTL::WebSocketInfo::CloseConnection(uint16_t statusCode, const CTL::String& reason) {
    // 构建关闭帧
    char closeFrame[2 + 125] = {0}; // 最大关闭帧大小为125字节
    closeFrame[0] = 0b10001000; // FIN=1, Opcode=0x8
    closeFrame[1] = 2; // 负载长度为2，表示只有状态码
    closeFrame[2] = 0x03; // 状态码高字节，1000表示正常关闭
    closeFrame[3] = 0xE8; // 状态码低字节
    // 发送关闭帧
    this->Write(closeFrame, 4);
    this->SetConnectFlag(false);
}

void WebSocketInfo::StatusUpdate() {
    m_count++;
}

void WebSocketInfo::StateReset() {
    m_count = 0;
}

int WebSocketInfo::GetStatus() const {
    return m_count;
}

void WebSocketInfo::SetURLPath(const String& path) {
    URL_Path = path;
}

String WebSocketInfo::GetURLPath() {
    return URL_Path;
}

bool WebSocketInfo::GetSendStatus() const {
    return m_status;
}

void CTL::WebSocket::AcceptingClient() {
    CTL::Socket client = m_sock.Accept();
    if (client.GetClientHost().Port == 0) {
        return;
    }
    // client.SetSocketNonBlocking();
    SSL* ssl = NewSSL(client.Socketbit);
    const CTL::String Data = GetRequest(client,ssl);
    M_S_POOl.AddTask(&WebSocket::ProcessingClientRequests,this,client,Data,ssl);
}

void CTL::WebSocket::ProcessingClientRequests(CTL::Socket& client,const CTL::String& Data,SSL* ssl) {
    // int value = 1;
    // setsockopt(client.Socketbit, SOL_SOCKET, MSL_N, &value, sizeof(value));
    ByteBuffer buffer;
    buffer.append(Data.data(),static_cast<int>(Data.size()));
    WebSocketPacket m_packet;
    auto info = new WebSocketInfo();
    info->ssl = ssl;
    const CCVar l = m_packet.recv_handshake(buffer);
    CTL::String URL = "";
    try {
        if (l == 0) {
            URL = m_packet.uri();
            CTL::String mod = m_packet.mothod();
            info->SetSock(client);
            const auto BURL = URLS[URL];
            if (!BURL) {
                client.Close();
                return;
            }
            CTL::String AnswerBack = URLS[URL] ? "OK" : "NO URL";
            const auto Answer = m_packet.pack_handshake_rsp(AnswerBack);
            if (Answer == 0) {
                info->Write(AnswerBack.data(),AnswerBack.size());
            }
            else {
                client.Close();
                return;
            }
            int ID = AllocationID();
            info->SetID(ID);
            info->StatusUpdate();
            info->SetURLPath(URL);
            AddClient(info);
            if (auto OpenFun = URLOpenFun[URL]) {
                if (OpenFun) {
                    try {
                        info->SetPacket(m_packet);
                        OpenFun(*info);
                    }
                    catch (CCException& exception) {
                        MessagePrint(exception.what());
                    }
                }
                else {
                    client.Close();
                    return;
                }
            }
            buffer.clear();
            m_packet.payload_.clear();
            bool is_client_error = false;
            while (ServerFlag) {
                bool F = true;
                if (ssl) {
                    F = !SSL_Client::IsShutDown(ssl);
                }
                if (client.isDataAvailable() && F) {
                    char buf[1024] = {0};
                    const auto length = ReadBuffer(ssl,client,buf,sizeof buf);
                    if (length > 0 && !is_client_error) {
                        if (length == -1) {
                            is_client_error = true;
                            continue;
                        }
                        buffer.append(buf,static_cast<int>(length));
                        uint64_t frameSize = m_packet.recv_dataframe(buffer);
                        if (frameSize > 0) {
                            // 成功接收到一个完整的帧
                            uint8_t opcode = m_packet.get_opcode();
                            if (opcode == WebSocketPacket::WSOpcode_Close) {
#ifdef _WIN32
                                if (info->GetConnectFlag()) {
                                    // 构建关闭帧
                                    char closeFrame[2 + 125]; // 最大关闭帧大小为125字节
                                    closeFrame[0] = 0b10001000; // FIN=1, Opcode=0x8
                                    closeFrame[1] = 2; // 负载长度为2，表示只有状态码
                                    closeFrame[2] = 0x03; // 状态码高字节，1000表示正常关闭
                                    closeFrame[3] = 0xE8; // 状态码低字节
                                    // 发送关闭帧
                                    info->Write(closeFrame, 4);
                                }
#endif

                                if (CCVar CloseFun = URLCloseFun[URL]) {
                                    if (CloseFun) {
                                        try {
                                            info->SetPacket(m_packet);
                                            info->SetConnectFlag(false);
                                            CloseFun(*info);
                                        }
                                        catch (CCException& exception) {
                                            MessagePrint(exception.what());
                                        }
                                    }
                                }
                                break;
                            }
                            else if (opcode == WebSocketPacket::WSOpcode_Binary || opcode == WebSocketPacket::WSOpcode_Text) {
                                if (CCVar MessageFun = URLMessageFun[URL]) {
                                    try {
                                        info->SetPacket(m_packet);
                                        // 获取payload数据
                                        ByteBuffer payload = m_packet.get_payload();
                                        info->SetBuffer(payload);
                                        MessageFun(*info);
                                    }
                                    catch (CCException& exception) {
                                        MessagePrint(exception.what());
                                    }
                                }
                            }
                            else if (opcode == WebSocketPacket::WSOpcode_Ping) {
                                if (CCVar PingFun = URL_Ping_Fun[URL]) {
                                    try {
                                        info->SetPacket(m_packet);
                                        PingFun(*info);
                                    }
                                    catch (CCException& exception) {
                                        MessagePrint(exception.what());
                                    }
                                }
                            }
                            else if (opcode == WebSocketPacket::WSOpcode_Pong) {
                                auto i = GetClient(ID);
                                if (i) {
                                    i->StatusUpdate();
                                }
                                if (CCVar PongFun = URL_Pong_Fun[URL]) {
                                    try {
                                        info->SetPacket(m_packet);
                                        PongFun(*info);
                                    }
                                    catch (CCException& exception) {
                                        MessagePrint(exception.what());
                                    }
                                }
                            }
                            buffer.clear();
                            m_packet.payload_.clear();
                        }
                    }
                    else {
                        if (CCVar ERFun = URL_ERROR_Fun[URL]) {
                            try {
                                if (ERFun) {
                                    info->SetPacket(m_packet);
                                    info->SetConnectFlag(false);
                                    ERFun(*info);
                                }
                            }
                            catch (CCException& e) {
                                MessagePrint("ERFun -> " + CTL::String(e.what()));
                            }
                        }
                        break;
                    }
                }
                else {
                    Thread::Sleep(1000);
                }
            }
        }
        else {
            MessagePrint("recv_handshake error");
        }
    }
    catch (CCException& exception) {
        const auto ERFun = URL_ERROR_Fun[URL];
        if (ERFun) {
            try {
                if (ERFun) {
                    info->SetPacket(m_packet);
                    info->SetConnectFlag(false);
                    ERFun(*info);
                }
            }
            catch (CCException& e) {
                MessagePrint("ERFun -> " + CTL::String(e.what()));
            }
        }
        else {
            MessagePrint("recv_handshake -> " + CTL::String(exception.what()));
        }
    }
    client.Close();
    if (ssl) {
        while (info->GetSendStatus()) {
            CTL::Thread::SleepMS(10);
        }
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
    }
    if (info) {
        DeleteClient(info->GetID());
    }
}

CCVector<int> CTL::WebSocket::GetClientListAll() {
    std::unique_lock<std::mutex> lock(m_mutex_t);
    CCVector<int> list_t;
    for (auto& item : Clients) {
        list_t.add_lock(item.first);
    }
    return list_t;
}

CTL::WebSocketInfo* CTL::WebSocket::GetClient(const int ID) {
    std::unique_lock<std::mutex> lock(m_mutex_t);
    const auto item = Clients.get(ID);
    if (item && *item) {
        const auto info = *item;
        return info;
    }
    return nullptr;
}

bool CTL::WebSocket::InitSSL() {
#ifdef _WIN32
    // OPENSSL_Applink();
#elif __linux__
    struct sigaction sa{};
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, nullptr);
#endif
    // 创建 SSL 上下文
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!ssl_ctx) {
        ERR_print_errors_fp(stderr);
        // 处理错误
        return false;
    }
    // 设置 SSL 选项，禁用不安全的协议版本
    SSL_CTX_set_options(ssl_ctx,
                        SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
                        SSL_OP_NO_COMPRESSION |
                        SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
    // 不校验客户端证书
    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, nullptr);
    return true;
} // Linux Debug 模式下可能会触发 SIGPIPE 普通运行不会有问题

bool CTL::WebSocket::LoadSSLCertificate(const CTL::String& CertPath) const {
    // 加载证书和密钥
    if (SSL_CTX_use_certificate_file(ssl_ctx, CertPath.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        // 处理错误
        return false;
    }
    return true;
}

bool CTL::WebSocket::LoadSSLKey(const CTL::String& KeyPath, const CTL::String& Passwd) const {
    void* passwd = nullptr;
    if(!Passwd.empty()) {
        passwd = (void*)Passwd.c_str();
    }
    if(passwd != nullptr)
        SSL_CTX_set_default_passwd_cb_userdata(ssl_ctx,passwd);
    if (SSL_CTX_use_PrivateKey_file(ssl_ctx, KeyPath.c_str(), SSL_FILETYPE_PEM) != 1) {
        ERR_print_errors_fp(stderr);
        // 处理错误
        return false;
    }
    // 判定私钥是否正确
    if(!SSL_CTX_check_private_key(ssl_ctx)) {
        printf("SSL_CTX_check_private_key error!\n");
        return false;
    }
    return true;
}

bool CTL::WebSocket::LoadSSLVerify(const CTL::String& CA_Path) const {
    // 加载CA的证书
    if(!SSL_CTX_load_verify_locations(ssl_ctx, CA_Path.c_str(), nullptr)) {
        printf("SSL_CTX_load_verify_locations error!\n");
        return false;
    }
    return true;
}

CTL::String CTL::WebSocket::GetRequest(CTL::Socket& client,SSL* ssl) {
    // client.SetSocketNonBlocking();
    if(ssl) {
        CTL::String data;
        while (true){
            char buffer[1024] = {0};
            const auto length = SSL_read(ssl, buffer, 1023);
            if(length > 0){
                for(auto i : buffer){
                    if(i != '\000') {
                        data.push_back(i);
                    }
                }
                auto l = data.find("\r\n\r\n");
                if(l != CTL::String::npos){
                    break;
                }
            }
            else{
                ERR_print_errors_fp(stderr);
                SSL_shutdown(ssl);
                SSL_free(ssl);
                return "";
            }
        }
        return data;
    }
    else {
        CTL::String data;
        int ET = 1;
        while (true){
            if(client.isDataAvailable()){
                char buffer[1] = {0};
                const auto length = client.RecvData(buffer,1);
                if(length > 0){
                    for(auto i : buffer){
                        if(i != '\000') {
                            data.push_back(i);
                        }
                    }
                    auto l = data.find("\r\n\r\n");
                    if(l != CTL::String::npos){
                        break;
                    }
                }
                else{
                    return {};
                }
            }
            else{
                if(ET >= 7){
                    break;
                }
                Thread::Sleep(1000 * ET * 10);
                ET++;
                continue;
                break;
            }
        }
        return data;
    }
}

void CTL::WebSocket::MessagePrint(const CTL::String& str) {
    CC::TimePrintln(str);
}

int CTL::WebSocket::AllocationID() {
    std::unique_lock<std::mutex> lock(m_mutex_t);
    for (int i = 0; i < 100000; i++) {
        if (!Clients.IsContains(i)) {
            return i;
        }
    }
    return -1;
}

void CTL::WebSocket::AddClient(WebSocketInfo* ws) {
    std::unique_lock<std::mutex> lock(m_mutex_t);
    ws->SetConnectFlag(true);
    Clients.put(ws->GetID(),ws);
}

SSL* CTL::WebSocket::NewSSL(const CTL::Socket& client) const {
    // 创建 SSL 对象并绑定到套接字
    SSL* ssl = SSL_new(this->ssl_ctx);
    if (!ssl) {
        return nullptr;
    }
    SSL_set_accept_state(ssl);
    SSL_set_fd(ssl, client.Socketbit);
    // 完成 SSL 握手
    // if (const int ret = SSL_accept(ssl); ret <= 0) {
    //     SSL_free(ssl);
    //     return nullptr;
    // }
    int code;
    int retryTimes = 0;
    while ((code = SSL_accept(ssl)) <= 0 && retryTimes++ < 100) {
        if (SSL_get_error(ssl, code) != SSL_ERROR_WANT_READ) {
        // printf("ssl accept error. %d\n", SSL_get_error(ssl, code));
           break;
        }
        Thread::Sleep(1000 * 100);
    }
    return ssl;
}

void CTL::WebSocket::DeleteClient(const int ID) {
    std::unique_lock<std::mutex> lock(m_mutex_t);
    const auto item = Clients.get(ID);
    if (item && *item) {
        auto info = *item;
        info->SetConnectFlag(false);
        Clients.Remove(ID);
        info = nullptr;
    }
}

ByteHander CTL::WebSocket::ReadBuffer(SSL* ssl,CTL::Socket& client,char* buf, ByteHander size) {
    ByteHander read = 0;
    if (ssl) {
        read = SSL_read(ssl, buf, size);
    }
    else {
        read = client.RecvData(buf,size);
    }
    return read;
}

void WebSocket::InitService() {
    if (Tick_m) {
        m_timer.Start([&](){Tick();}, m_timer_count, TimeType::Second);
    }
    ServerFlag = true;
}

void WebSocket::Stop() {
    ServerFlag = false;
    event_loop_t.stop();
    m_timer.Stop();
    m_sock.Close();
}

std::string &WebSocket::trim(std::string &str) {
    if (str.empty()) {
        return str;
    }

    // 去除前导空格
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));

    // 去除尾随空格
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), str.end());

    return str;
}

bool CTL::WebSocketClient::InitSSL() {
#ifdef _WIN32
    // OPENSSL_Applink();
#elif __linux__
    struct sigaction sa{};
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, nullptr);
#endif
    // 创建 SSL 上下文
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!ssl_ctx) {
        ERR_print_errors_fp(stderr);
        // 处理错误
        return false;
    }
    // 设置 SSL 选项，禁用不安全的协议版本
    SSL_CTX_set_options(ssl_ctx,
                        SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
                        SSL_OP_NO_COMPRESSION |
                        SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
    // 不校验客户端证书
    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, nullptr);
    return true;
} // Linux Debug 模式下可能会触发 SIGPIPE 普通运行不会有问题

bool CTL::WebSocketClient::LoadSSLCertificate(const CTL::String& CertPath) const {
    // 加载证书和密钥
    if (SSL_CTX_use_certificate_file(ssl_ctx, CertPath.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        // 处理错误
        return false;
    }
    return true;
}

bool CTL::WebSocketClient::LoadSSLKey(const CTL::String& KeyPath, const CTL::String& Passwd) const {
    void* passwd = nullptr;
    if(!Passwd.empty()) {
        passwd = (void*)Passwd.c_str();
    }
    if(passwd != nullptr)
        SSL_CTX_set_default_passwd_cb_userdata(ssl_ctx,passwd);
    if (SSL_CTX_use_PrivateKey_file(ssl_ctx, KeyPath.c_str(), SSL_FILETYPE_PEM) != 1) {
        ERR_print_errors_fp(stderr);
        // 处理错误
        return false;
    }
    // 判定私钥是否正确
    if(!SSL_CTX_check_private_key(ssl_ctx)) {
        printf("SSL_CTX_check_private_key error!\n");
        return false;
    }
    return true;
}

bool CTL::WebSocketClient::LoadSSLVerify(const CTL::String& CA_Path) const {
    // 加载CA的证书
    if(!SSL_CTX_load_verify_locations(ssl_ctx, CA_Path.c_str(), nullptr)) {
        printf("SSL_CTX_load_verify_locations error!\n");
        return false;
    }
    return true;
}

bool WebSocketInfo::SendPing(const ByteArray& binaryData) {
    const size_t payload_length = binaryData.size();
    const auto frame = new char[payload_length + 2];
    frame[0] = 0b10001001;
    // 设置Mask位为payload_length，负载长度
    frame[1] = static_cast<char>(payload_length);
    memcpy(frame + 2, binaryData.buffer(), payload_length);
    const bool F = this->Write(frame, payload_length + 2);
    delete[] frame;
    return F;
}

bool WebSocketInfo::SendPong(const ByteArray& binaryData) {
    const size_t payload_length = binaryData.size();
    const auto frame = new char[payload_length + 2];
    frame[0] = 0b10001010;
    // 设置Mask位为payload_length，负载长度
    frame[1] = static_cast<char>(payload_length);
    memcpy(frame + 2, binaryData.buffer(), payload_length);
    const bool F = this->Write(frame, payload_length + 2);
    delete[] frame;
    return F;
}

void WebSocket::Tick() {
    const auto List = Clients.values();
    for (const auto& i : List) {
        if (i) {
            const auto count = i->GetStatus();
            if (count == 0) {
                i->CloseConnection();
                DeleteClient(i->GetID());
            }
            else {
                i->SendPing("");
                i->StateReset();
            }
        }
    }
}

void WebClient::write(const CTL::ByteArray &data) const {
    if (!client) {
        return;
    }
    client->write(data);
}

void WebClient::set_client(EPClient *cl) {
    this->client = cl;
}

void WebClient::close() {
    SetConnectFlag(false);
    if (client) {
        client->close();
    }
}

bool WebClient::Write(const char *data,const size_t size) {
    write(CTL::ByteArray(data,size));
    return true;
}

WebSocketServer::WebSocketServer() {
    m_server = new CTL::EPServer();
}

WebSocketServer::~WebSocketServer() {
    if (m_server) {
        delete m_server;
        m_server = nullptr;
    }
}

void WebSocketServer::setEPSocket(EPServer* server_t) {
    m_server = server_t;
    if (m_server) {
        Flag = true;
    }
}

bool WebSocketServer::Init(const CTL::String& ip,const int port) {
    if (!m_server) {
        return false;
    }
    Flag = m_server->listenInit(ip,port);
    return Flag;
}

bool WebSocketServer::InitBind(const WSBind& bind) {
    this->OnBind(bind.URL, bind.OnOpenFun, bind.OnCloseFun, bind.OnMessageFun,
        bind.OnErrorFun, bind.OnPingFun, bind.OnPongFun);
    return true;
}

bool WebSocketServer::InitSSL(const std::string& cert_file, const std::string& key_file, const std::string& pass) const {
    if (!m_server) {
        return false;
    }
    return m_server->initSSL(cert_file, key_file, pass);
}

void WebSocketServer::OnBind(const CTL::String& URL, const RequestFunc2& OnOpenFun, const RequestFunc2& OnCloseFun,
                             const RequestFunc2& OnMessageFun, const RequestFunc2& OnErrorFun, const RequestFunc2& OnPingFun,
                             const RequestFunc2& OnPongFun) {
    if (!m_server) {
        return;
    }
    URLS[URL] = true;
    URLOpenFun.insert(std::pair<CTL::String, RequestFunc2>(URL, OnOpenFun));
    URLCloseFun.insert(std::pair<CTL::String, RequestFunc2>(URL, OnCloseFun));
    URLMessageFun.insert(std::pair<CTL::String, RequestFunc2>(URL, OnMessageFun));
    URL_ERROR_Fun.insert(std::pair<CTL::String, RequestFunc2>(URL, OnErrorFun));
    URL_Ping_Fun.insert(std::pair<CTL::String, RequestFunc2>(URL, OnPingFun));
    URL_Pong_Fun.insert(std::pair<CTL::String, RequestFunc2>(URL, OnPongFun));
}

void WebSocketServer::HandleClient(const SOCKET &fd,const CTL::String& request) {
    if (!m_server) {
        return;
    }
    const auto client_t = m_server->getClient(fd);
    const auto info = new WebClient();
    info->id = fd;
    info->set_client(client_t);
    info->ssl = client_t->getSSL();
    put(info);
    info->SetConnectFlag(true);
    if (client_t) {
        ByteBuffer buffer;
        buffer.append(request.data(),static_cast<int>(request.size()));
        WebSocketPacket m_packet;
        const auto l = m_packet.recv_handshake(buffer);
        CTL::String URL = "";
        CTL::AutoDestruct<WebSocketInfo> client_in_d(info);
        client_in_d.set_destroy_func([&]() {
            remove(info);
        });
        if (l == 0) {
            URL = m_packet.uri();
            CTL::String mod = m_packet.mothod();
            const auto BURL = URLS[URL];
            if (!BURL) {
                client_t->close();
                return;
            }
            CTL::String AnswerBack = URLS[URL] ? "OK" : "NO URL";
            const auto Answer = m_packet.pack_handshake_rsp(AnswerBack);
            if (Answer == 0) {
                client_t->write(AnswerBack);
            }
            else {
                client_t->close();
                return;
            }
            info->SetID(fd);
            info->StatusUpdate();
            info->SetURLPath(URL);
            if (auto OpenFun = URLOpenFun[URL]) {
                if (OpenFun) {
                    try {
                        OpenFun(*info);
                    }
                    catch (CCException& exception) {

                    }
                }
                else {
                    client_t->close();
                    return;
                }
            }
            buffer.clear();
            m_packet.payload_.clear();
            bool is_client_error = false;
            while (Flag && info->GetConnectFlag()) {
                if (client_t->is_recv_readable()) {
                    const auto data = client_t->recv();
                    buffer.append(data.data.buffer(),static_cast<int>(data.data.size()));
                    uint64_t frameSize = m_packet.recv_dataframe(buffer);
                    if (frameSize > 0) {
                        const uint8_t opcode = m_packet.get_opcode();
                        if (opcode == WebSocketPacket::WSOpcode_Close) {
#ifdef _WIN32
                            if (info->GetConnectFlag()) {
                                // 构建关闭帧
                                char closeFrame[2 + 125]; // 最大关闭帧大小为125字节
                                closeFrame[0] = 0b10001000; // FIN=1, Opcode=0x8
                                closeFrame[1] = 2; // 负载长度为2，表示只有状态码
                                closeFrame[2] = 0x03; // 状态码高字节，1000表示正常关闭
                                closeFrame[3] = 0xE8; // 状态码低字节
                                // 发送关闭帧
                                info->Write(closeFrame, 4);
                            }
#endif

                            if (auto CloseFun = URLCloseFun[URL]) {
                                if (CloseFun) {
                                    try {
                                        info->SetPacket(m_packet);
                                        info->SetConnectFlag(false);
                                        CloseFun(*info);
                                    }
                                    catch (CCException& exception) {

                                    }
                                }
                            }
                            break;
                        }
                        else if (opcode == WebSocketPacket::WSOpcode_Binary || opcode == WebSocketPacket::WSOpcode_Text) {
                            if (auto MessageFun = URLMessageFun[URL]) {
                                try {
                                    info->SetPacket(m_packet);
                                    ByteBuffer payload = m_packet.get_payload();
                                    info->SetBuffer(payload);
                                    MessageFun(*info);
                                }
                                catch (CCException& exception) {

                                }
                            }
                        }
                        else if (opcode == WebSocketPacket::WSOpcode_Ping) {
                            info->StatusUpdate();
                            if (auto PingFun = URL_Ping_Fun[URL]) {
                                try {
                                    info->SetPacket(m_packet);
                                    PingFun(*info);
                                }
                                catch (CCException& exception) {

                                }
                            }
                        }
                        else if (opcode == WebSocketPacket::WSOpcode_Pong) {
                            info->StatusUpdate();
                            if (auto PongFun = URL_Pong_Fun[URL]) {
                                try {
                                    info->SetPacket(m_packet);
                                    PongFun(*info);
                                }
                                catch (CCException& exception) {
                                    // MessagePrint(exception.what());
                                }
                            }
                        }
                        buffer.clear();
                        m_packet.payload_.clear();
                    }
                }
                else {
                    Thread::SleepMS(10);
                }
            }
        }
    }
}

void WebSocketServer::InitTick(const unsigned long tick_time) {
    if (tick_time > 0) {
        m_timer.Start([&](){Tick();}, tick_time, TimeType::Second);
    }
}

int WebSocketServer::Running() {
    if (!m_server) {
        return -1;
    }
    if (!Init_Bind) {
        m_server->setOpenFunc([this](const SOCKET& fd) {
            const auto client = getClient(fd);
            if (!client) {
                CTL::Thread::addTask([this](const SOCKET& fd) {
                    const auto Data = getRequest(fd);
                    HandleClient(fd,Data);
                },fd);
            }
        });
        // m_server->setMessageFunc([this](const SOCKET& fd) {
        //
        // });
        m_server->setCloseFunc([this](const SOCKET& fd) {
            const auto client = getClient(fd);
            if (client) {
                client->SetConnectFlag(false);
            }
        });
        Init_Bind = true;
    }
    return m_server->running();
}

void WebSocketServer::Stop() {
    Flag = false;
    Init_Bind = false;
    m_timer.Stop();
    if (m_server) {
        m_server->stop();
    }
}

WebClient * WebSocketServer::getClient(const SOCKET &id) {
    CCUniqueLock lock(m_mutex_t);
    return Clients[id];
}

std::vector<SOCKET> WebSocketServer::getClientFds() const {
    CCUniqueLock lock(m_mutex_t);
    std::vector<SOCKET> fds;
    for (const auto& client : Clients) {
        fds.push_back(client.first);
    }
    return fds;
}

void WebSocketServer::put(WebClient *client) {
    if (!client) {
        return;
    }
    CCUniqueLock lock(m_mutex_t);
    Clients[client->id] = client;
}

void WebSocketServer::remove(WebClient *client) {
    if (!client) {
        return;
    }
    CCUniqueLock lock(m_mutex_t);
    const auto fd = client->id;
    client->close();
    Clients.erase(fd);
}

CTL::String WebSocketServer::getRequest(const SOCKET &id) const {
    CTL::String data;
    const auto client = m_server->getClient(id);
    if (!client) {
        return "";
    }
    while (true){
        const auto buffer = client->recv(1024);
        if(buffer.data.size() > 0){
            for(char i : buffer.data){
                if(i != '\000') {
                    data.push_back(i);
                }
            }
            const auto l = data.find("\r\n\r\n");
            if(l != CTL::String::npos){
                break;
            }
        }
        else{
            return "";
        }
    }
    return data;
}

void WebSocketServer::Tick() {
    const auto List = Clients.values();
    for (const auto& i : List) {
        if (i) {
            const auto count = i->GetStatus();
            if (count <= 0) {
                i->CloseConnection();
                // remove(i);
            }
            else {
                i->SendPing("");
                i->StateReset();
            }
        }
    }
}




