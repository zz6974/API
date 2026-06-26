#ifndef SRCCTL_CC_SMTP_H
#define SRCCTL_CC_SMTP_H

#include "Socket/CCSocket.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>

namespace CTL {

    enum class SmtpSecurity {
        NONE,       // 无加密 (端口 25)
        STARTTLS,   // 使用 STARTTLS 升级加密 (端口 587)
        SSL_TLS     // 直接 SSL/TLS (端口 465)
    };

    struct EmailMessage {
        std::string from;
        std::string fromName;
        std::string to;
        std::vector<std::string> cc;
        std::vector<std::string> bcc;
        std::string subject;
        std::string body;
        std::string contentType = "text/plain";  // text/plain 或 text/html
        std::string charset = "UTF-8";
        std::vector<std::string> attachments;  // 文件路径列表

        EmailMessage() = default;
    };

    class SmtpClient {
    private:
        std::string smtpHost_;
        int smtpPort_;
        std::string username_;
        std::string password_;
        SmtpSecurity security_;
        bool useAuth_;
        bool debugMode_ = false;  // 调试模式开关

        std::unique_ptr<Socket> socket_;

        // SSL/TLS 相关
        SSL_CTX* sslCtx_ = nullptr;
        SSL* ssl_ = nullptr;
        bool isSslConnected_ = false;

        char recvBuffer_[4096]{};
        std::string lastError_;
        int lastErrorCode_ = 0;

        // 初始化 SSL 上下文
        bool initSSL();

        // 清理 SSL 资源
        void cleanupSSL();

        // 执行 SSL 握手
        bool performSSLHandshake();

        // Base64 编码
        static std::string base64Encode(const std::string& input);

        // 接收服务器响应（支持 SSL）
        int receiveResponse(std::string& response, int expectedCode = -1);

        // 发送命令并检查响应（支持 SSL）
        bool sendCommand(const std::string& command, int expectedCode);

        // 建立连接
        bool connectToServer();

        // 断开连接
        void disconnect();

        // 构建邮件内容
        std::string buildEmailContent(const EmailMessage& msg);

        // 设置错误信息
        void setError(int code, const std::string& message) {
            lastErrorCode_ = code;
            lastError_ = message;
        }

        // 调试输出辅助函数
        void debugPrint(const std::string& message) const {
            if (debugMode_) {
                std::cout << message << std::flush;
            }
        }

    public:
        // 构造函数
        SmtpClient(const std::string& host, int port, SmtpSecurity security = SmtpSecurity::STARTTLS);

        // 析构函数
        ~SmtpClient();

        // 禁止拷贝
        SmtpClient(const SmtpClient&) = delete;
        SmtpClient& operator=(const SmtpClient&) = delete;

        // 设置认证信息
        void setAuth(const std::string& username, const std::string& password);

        // 设置调试模式
        void setDebugMode(bool enable) { debugMode_ = enable; }

        // 获取调试模式状态
        bool getDebugMode() const { return debugMode_; }

        // 发送邮件（简化版）
        bool sendEmail(const std::string& from,
                      const std::string& to,
                      const std::string& subject,
                      const std::string& body);

        // 发送邮件（完整版）
        bool sendEmail(const EmailMessage& msg);

        // 获取最后错误信息
        [[nodiscard]] std::string getLastError() const { return lastError_; }

        // 获取最后错误码
        int getLastErrorCode() const { return lastErrorCode_; }

        // 测试连接
        bool testConnection();
    };

}

#endif
