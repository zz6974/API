#include "SMTP/CCSMTP.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "CCThread.h"

using namespace CTL;

// ==================== 初始化 SSL 上下文 ====================
bool SmtpClient::initSSL() {
    debugPrint("[SSL] Initializing SSL context...\n");

    // 初始化 OpenSSL
    OPENSSL_init_ssl(OPENSSL_INIT_SSL_DEFAULT, nullptr);

    // 创建 SSL 上下文
    const SSL_METHOD* method = TLS_client_method();
    sslCtx_ = SSL_CTX_new(method);
    if (!sslCtx_) {
        setError(-10, "Failed to create SSL context");
        std::cerr << "[SSL ERROR] " << lastError_ << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // 设置验证模式（可选：不验证证书用于测试）
    // SSL_CTX_set_verify(sslCtx_, SSL_VERIFY_NONE, nullptr);

    // 设置最低 TLS 版本为 1.2
    SSL_CTX_set_min_proto_version(sslCtx_, TLS1_2_VERSION);

    debugPrint("[SSL] SSL context initialized successfully\n");
    return true;
}

// ==================== 清理 SSL 资源 ====================
void SmtpClient::cleanupSSL() {
    if (ssl_) {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
    if (sslCtx_) {
        SSL_CTX_free(sslCtx_);
        sslCtx_ = nullptr;
    }
    isSslConnected_ = false;
    debugPrint("[SSL] SSL resources cleaned up\n");
}

// ==================== 执行 SSL 握手 ====================
bool SmtpClient::performSSLHandshake() {
    debugPrint("[SSL] Starting SSL handshake...\n");

    // 创建 SSL 对象
    ssl_ = SSL_new(sslCtx_);
    if (!ssl_) {
        setError(-11, "Failed to create SSL object");
        std::cerr << "[SSL ERROR] " << lastError_ << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // 将 SSL 关联到 socket
    if (SSL_set_fd(ssl_, socket_->Socketbit) != 1) {
        setError(-12, "Failed to set SSL file descriptor");
        std::cerr << "[SSL ERROR] " << lastError_ << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // 设置 SNI (Server Name Indication)
    SSL_set_tlsext_host_name(ssl_, smtpHost_.c_str());

    // 执行 SSL 连接
    int ret = SSL_connect(ssl_);
    if (ret != 1) {
        int sslError = SSL_get_error(ssl_, ret);
        setError(-13, "SSL handshake failed with error code: " + std::to_string(sslError));
        std::cerr << "[SSL ERROR] " << lastError_ << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    isSslConnected_ = true;

    // 打印加密信息
    debugPrint("[SSL] Handshake successful!\n");
    debugPrint("[SSL] Using cipher: " + std::string(SSL_get_cipher(ssl_)) + "\n");
    debugPrint("[SSL] Protocol version: " + std::string(SSL_get_version(ssl_)) + "\n");

    return true;
}

// ==================== Base64 编码实现 ====================
std::string SmtpClient::base64Encode(const std::string& input) {
    static const char base64_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    int in_len = input.size();
    const char* bytes_to_encode = input.c_str();

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; i < 4; i++)
                result += base64_table[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(int j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int j = 0; j < i + 1; j++)
            result += base64_table[char_array_4[j]];

        while(i++ < 3)
            result += '=';
    }

    return result;
}

// ==================== 接收服务器响应（支持 SSL）====================
int SmtpClient::receiveResponse(std::string& response, int expectedCode) {
    memset(recvBuffer_, 0, sizeof(recvBuffer_));

    int bytesRead = 0;

    if (isSslConnected_ && ssl_) {
        // 使用 SSL 读取
        debugPrint("[DEBUG] Using SSL_read\n");
        bytesRead = SSL_read(ssl_, recvBuffer_, sizeof(recvBuffer_) - 1);
        if (bytesRead <= 0) {
            int sslError = SSL_get_error(ssl_, bytesRead);
            setError(-14, "SSL read failed with error: " + std::to_string(sslError));
            std::cerr << "[SSL ERROR] " << lastError_ << std::endl;
            ERR_print_errors_fp(stderr);
            return 0;
        }
    } else {
        // 使用普通 socket 读取，增加重试机制
        debugPrint("[DEBUG] Using regular socket read\n");

        for (int attempt = 0; attempt < 3; ++attempt) {
            fd_set readfds;
            struct timeval timeout;
            timeout.tv_sec = 5;  // 每次等待 5 秒
            timeout.tv_usec = 0;

            FD_ZERO(&readfds);
            FD_SET(socket_->Socketbit, &readfds);

            debugPrint("[DEBUG] Select attempt " + std::to_string(attempt + 1) + "\n");
            int ret = select(socket_->Socketbit + 1, &readfds, nullptr, nullptr, &timeout);

            if (ret < 0) {
                if (errno == EINTR) {
                    debugPrint("[DEBUG] Select interrupted, retrying...\n");
                    continue;
                }
                setError(-1, "Select failed: " + std::string(strerror(errno)));
                std::cerr << "[SMTP ERROR] " << lastError_ << std::endl;
                return -1;
            } else if (ret == 0) {
                debugPrint("[DEBUG] Select timeout on attempt " + std::to_string(attempt + 1) + "\n");
                continue;  // 重试
            }

            // 有数据可读
            // bytesRead = recv(socket_->Socketbit, recvBuffer_, sizeof(recvBuffer_) - 1, MSG_DONTWAIT);
            bytesRead = socket_->RecvData(recvBuffer_, sizeof(recvBuffer_) - 1);
            if (bytesRead > 0) {
                break;  // 成功读取
            }
            else if (bytesRead == 0) {
                setError(-3, "Connection closed by server");
                std::cerr << "[SMTP ERROR] " << lastError_ << std::endl;
                return 0;
            }
            else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    debugPrint("[DEBUG] No data yet, retrying...\n");
                    CTL::Thread::Sleep(100000);  // 等待 100ms
                    continue;
                }
                setError(-3, "Recv error: " + std::string(strerror(errno)));
                std::cerr << "[SMTP ERROR] " << lastError_ << std::endl;
                return 0;
            }
        }

        if (bytesRead <= 0) {
            setError(-2, "Receive timeout after multiple attempts");
            std::cerr << "[SMTP ERROR] " << lastError_ << std::endl;
            return 0;
        }
    }

    recvBuffer_[bytesRead] = '\0';
    response = std::string(recvBuffer_);

    debugPrint("[SMTP RECV] " + response + "\n");

    // 检查响应码
    if (expectedCode > 0 && bytesRead >= 3) {
        int responseCode = atoi(recvBuffer_);
        if (responseCode != expectedCode) {
            setError(responseCode, "Unexpected response code " + std::to_string(responseCode) +
                           ", expected " + std::to_string(expectedCode) + ": " + response);
            std::cerr << "[SMTP ERROR] " << lastError_ << std::endl;
            return responseCode;
        }
    }

    return 1;
}

// ==================== 发送命令（支持 SSL）====================
bool SmtpClient::sendCommand(const std::string& command, int expectedCode) {
    debugPrint("[SMTP SEND] " + command + "\n");

    bool sendSuccess = false;

    if (isSslConnected_ && ssl_) {
        // 使用 SSL 发送
        int sent = SSL_write(ssl_, command.c_str(), command.size());
        sendSuccess = (sent > 0);
        if (!sendSuccess) {
            int sslError = SSL_get_error(ssl_, sent);
            setError(-4, "SSL write failed: " + std::to_string(sslError));
            std::cerr << "[SSL ERROR] " << lastError_ << std::endl;
            ERR_print_errors_fp(stderr);
        }
    } else {
        // 使用普通 socket 发送
        sendSuccess = socket_->SendByte(command.c_str(), command.size());
        if (!sendSuccess) {
            setError(-4, "Failed to send command");
            std::cerr << "[SMTP ERROR] " << lastError_ << std::endl;
        }
    }

    if (!sendSuccess) {
        return false;
    }

    // 等待一小段时间让服务器处理
    CTL::Thread::Sleep(100000);  // 100ms

    std::string response;
    int result = receiveResponse(response, expectedCode);
    return result > 0;
}

// ==================== 连接服务器 ====================
bool SmtpClient::connectToServer() {
    debugPrint("[SMTP] Connecting to " + smtpHost_ + ":" + std::to_string(smtpPort_) +
              " (Security: " + (security_ == SmtpSecurity::NONE ? "NONE" :
                                security_ == SmtpSecurity::STARTTLS ? "STARTTLS" : "SSL/TLS") +
              ")\n");

    socket_ = std::make_unique<Socket>();

    // 初始化 Socket
    if (!socket_->Init(IPVX::IPV4, TORU::TCP)) {
        setError(-5, "Failed to initialize socket");
        std::cerr << "[SMTP ERROR] " << lastError_ << std::endl;
        return false;
    }

    // 连接到服务器
    if (!socket_->Connect(smtpHost_.c_str(), smtpPort_)) {
        setError(-6, "Failed to connect to " + smtpHost_ + ":" + std::to_string(smtpPort_));
        std::cerr << "[SMTP ERROR] " << lastError_ << std::endl;
        return false;
    }

    debugPrint("[SMTP] TCP connected successfully\n");

    // 如果是 SSL/TLS 模式（端口 465），立即进行 SSL 握手
    if (security_ == SmtpSecurity::SSL_TLS) {
        debugPrint("[SMTP] Initializing SSL for direct TLS connection...\n");
        if (!initSSL()) {
            return false;
        }
        if (!performSSLHandshake()) {
            disconnect();
            return false;
        }
    }

    // 等待一小段时间，让服务器发送欢迎信息
    debugPrint("[SMTP] Waiting for server greeting...\n");
    CTL::Thread::Sleep(200000);  // 200ms

    // 接收欢迎信息（220）
    std::string response;
    if (receiveResponse(response, 220) <= 0) {
        std::cerr << "[SMTP] Failed to receive server greeting" << std::endl;
        return false;
    }

    debugPrint("[SMTP] Server greeting received successfully\n");
    return true;
}

// ==================== 断开连接 ====================
void SmtpClient::disconnect() {
    cleanupSSL();
    if (socket_) {
        socket_->Close();
        socket_.reset();
        debugPrint("[SMTP] Disconnected\n");
    }
}

// ==================== 构建邮件内容 ====================
std::string SmtpClient::buildEmailContent(const EmailMessage& msg) {
    std::ostringstream oss;

    // 邮件头
    if (!msg.fromName.empty()) {
        oss << "From: \"" << msg.fromName << "\" <" << msg.from << ">\r\n";
    } else {
        oss << "From: <" << msg.from << ">\r\n";
    }

    oss << "To: <" << msg.to << ">\r\n";

    // 抄送
    if (!msg.cc.empty()) {
        oss << "Cc: ";
        for (size_t i = 0; i < msg.cc.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << "<" << msg.cc[i] << ">";
        }
        oss << "\r\n";
    }

    oss << "Subject: " << msg.subject << "\r\n";
    oss << "MIME-Version: 1.0\r\n";

    if (msg.contentType == "text/html") {
        oss << "Content-Type: text/html; charset=\"" << msg.charset << "\"\r\n";
    } else {
        oss << "Content-Type: text/plain; charset=\"" << msg.charset << "\"\r\n";
    }

    oss << "Content-Transfer-Encoding: 7bit\r\n";
    oss << "\r\n";

    // 邮件正文
    oss << msg.body << "\r\n";

    return oss.str();
}

// ==================== 构造函数 ====================
SmtpClient::SmtpClient(const std::string& host, int port, SmtpSecurity security)
    : smtpHost_(host)
    , smtpPort_(port)
    , security_(security)
    , useAuth_(false)
    , debugMode_(false)  // 默认关闭调试模式
{
    #ifdef _MSC_VER
        WORD wVersionRequested;
        WSADATA wsaData;
        wVersionRequested = MAKEWORD(2, 2);
        WSAStartup(wVersionRequested, &wsaData);
    #endif
}

// ==================== 析构函数 ====================
SmtpClient::~SmtpClient() {
    disconnect();
    #ifdef _MSC_VER
        WSACleanup();
    #endif
}

// ==================== 设置认证信息 ====================
void SmtpClient::setAuth(const std::string& username, const std::string& password) {
    username_ = username;
    password_ = password;
    useAuth_ = true;
}

// ==================== 发送邮件（简化版）====================
bool SmtpClient::sendEmail(const std::string& from,
                          const std::string& to,
                          const std::string& subject,
                          const std::string& body) {
    EmailMessage msg;
    msg.from = from;
    msg.to = to;
    msg.subject = subject;
    msg.body = body;
    return sendEmail(msg);
}

// ==================== 发送邮件（完整版）====================
bool SmtpClient::sendEmail(const EmailMessage& msg) {
    if (debugMode_) {
        std::cout << "\n========== Starting Email Send ==========" << std::endl;
    }

    // 1. 连接服务器
    if (!connectToServer()) {
        std::cerr << "[SMTP] Failed at connection stage" << std::endl;
        return false;
    }

    // 2. 发送 EHLO
    if (debugMode_) {
        std::cout << "\n[Step 2] Sending EHLO..." << std::endl;
    }
    if (!sendCommand("EHLO localhost\r\n", 250)) {
        std::cerr << "[SMTP] Failed at EHLO stage" << std::endl;
        disconnect();
        return false;
    }

    // 3. 如果需要 STARTTLS
    if (security_ == SmtpSecurity::STARTTLS) {
        if (debugMode_) {
            std::cout << "\n[Step 3] Upgrading to STARTTLS..." << std::endl;
        }

        if (!sendCommand("STARTTLS\r\n", 220)) {
            std::cerr << "[SMTP] Server does not support STARTTLS or failed" << std::endl;
            disconnect();
            return false;
        }

        // 初始化 SSL 并执行握手
        if (!initSSL()) {
            disconnect();
            return false;
        }

        if (!performSSLHandshake()) {
            disconnect();
            return false;
        }

        // SSL 握手成功后，重新发送 EHLO
        if (debugMode_) {
            std::cout << "\n[Step 3b] Re-sending EHLO after TLS upgrade..." << std::endl;
        }
        if (!sendCommand("EHLO localhost\r\n", 250)) {
            std::cerr << "[SMTP] Failed at post-TLS EHLO stage" << std::endl;
            disconnect();
            return false;
        }
    }

    // 4. 认证
    if (useAuth_) {
        if (debugMode_) {
            std::cout << "\n[Step 4] Authenticating..." << std::endl;
        }

        if (!sendCommand("AUTH LOGIN\r\n", 334)) {
            std::cerr << "[SMTP] Failed at AUTH LOGIN stage" << std::endl;
            disconnect();
            return false;
        }

        // 发送用户名（Base64 编码）
        std::string encodedUser = base64Encode(username_);
        debugPrint("[DEBUG] Encoded username length: " + std::to_string(encodedUser.size()) + "\n");
        if (!sendCommand(encodedUser + "\r\n", 334)) {
            std::cerr << "[SMTP] Failed at username authentication" << std::endl;
            disconnect();
            return false;
        }

        // 发送密码（Base64 编码）
        std::string encodedPass = base64Encode(password_);
        debugPrint("[DEBUG] Encoded password length: " + std::to_string(encodedPass.size()) + "\n");
        if (!sendCommand(encodedPass + "\r\n", 235)) {
            std::cerr << "[SMTP] Failed at password authentication" << std::endl;
            disconnect();
            return false;
        }

        if (debugMode_) {
            std::cout << "[SMTP] Authentication successful!" << std::endl;
        }
    }

    // 5. MAIL FROM
    if (debugMode_) {
        std::cout << "\n[Step 5] Setting sender..." << std::endl;
    }
    std::string mailFrom = "MAIL FROM:<" + msg.from + ">\r\n";
    if (!sendCommand(mailFrom, 250)) {
        std::cerr << "[SMTP] Failed at MAIL FROM stage" << std::endl;
        disconnect();
        return false;
    }

    // 6. RCPT TO
    if (debugMode_) {
        std::cout << "\n[Step 6] Setting recipient..." << std::endl;
    }
    std::string rcptTo = "RCPT TO:<" + msg.to + ">\r\n";
    if (!sendCommand(rcptTo, 250)) {
        std::cerr << "[SMTP] Failed at RCPT TO stage" << std::endl;
        disconnect();
        return false;
    }

    // 7. DATA
    if (debugMode_) {
        std::cout << "\n[Step 7] Starting data transmission..." << std::endl;
    }
    if (!sendCommand("DATA\r\n", 354)) {
        std::cerr << "[SMTP] Failed at DATA stage" << std::endl;
        disconnect();
        return false;
    }

    // 8. 发送邮件内容
    if (debugMode_) {
        std::cout << "\n[Step 8] Sending email content..." << std::endl;
    }
    std::string emailContent = buildEmailContent(msg);
    debugPrint("[SMTP SEND] " + emailContent + "\n");

    bool sendSuccess = false;
    if (isSslConnected_ && ssl_) {
        int sent = SSL_write(ssl_, emailContent.c_str(), emailContent.size());
        sendSuccess = (sent > 0);
    } else {
        sendSuccess = socket_->SendByte(emailContent.c_str(), emailContent.size());
    }

    if (!sendSuccess) {
        setError(-7, "Failed to send email content");
        std::cerr << "[SMTP ERROR] " << lastError_ << std::endl;
        disconnect();
        return false;
    }

    // 9. 结束邮件
    if (debugMode_) {
        std::cout << "\n[Step 9] Finalizing email..." << std::endl;
    }
    if (!sendCommand("\r\n.\r\n", 250)) {
        std::cerr << "[SMTP] Failed to finalize email" << std::endl;
        disconnect();
        return false;
    }

    // 10. QUIT
    if (debugMode_) {
        std::cout << "\n[Step 10] Closing session..." << std::endl;
    }
    sendCommand("QUIT\r\n", 221);

    // 11. 断开连接
    disconnect();

    if (debugMode_) {
        std::cout << "\n========== Email Sent Successfully! ==========" << std::endl;
    }
    return true;
}

// ==================== 测试连接 ====================
bool SmtpClient::testConnection() {
    if (debugMode_) {
        std::cout << "\n[TEST] Testing connection to " << smtpHost_ << ":" << smtpPort_ << std::endl;
    }

    if (!connectToServer()) {
        return false;
    }

    if (!sendCommand("EHLO localhost\r\n", 250)) {
        disconnect();
        return false;
    }

    sendCommand("QUIT\r\n", 221);
    disconnect();

    if (debugMode_) {
        std::cout << "[TEST] Connection test passed!" << std::endl;
    }
    return true;
}
