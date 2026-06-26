#ifndef CC_SSL_H
#define CC_SSL_H

#include "openssl/ssl.h"
#include "openssl/err.h"
#include "CCString.h"
#include "Socket/CCClientSocket.h"
#include <vector>

namespace CTL {
    struct Cert_t {
        int Day;
        CTL::String SerialNumber;
        CTL::String ValidityTime;
        CTL::String Issuer;
    };
    class SSL_Client {
        ssl_st *ssl = nullptr;
    public:
    private:
    public:
        SSL_Client(ssl_st *ssl);
        ~SSL_Client();
        ByteHander read(char* buffer,size_t size) const;
        ByteHander read_ex(char* buffer,size_t size) const;
        int write(const char* buffer,size_t size) const;
        SSL* getSSL() const;
        static bool IsShutDown(const ssl_st *ssl);
        void release();
        bool available() const;
    };
    class SSL_BIO {
        BIO* r_bio,* w_bio;
    public:
    private:
    public:
        SSL_BIO(BIO* r_bio,BIO* w_bio);
        ~SSL_BIO();
    };
    class CTX_SSL {
        SSL_CTX* ssl_ctx = nullptr;
        bool is_Async = false;
        bool m_isServer = true;  // true=服务端, false=客户端
        CTL::String m_hostName;  // 客户端 SNI 主机名
    public:
    private:
    public:
        CTX_SSL();
        explicit CTX_SSL(bool Server);
        ~CTX_SSL();
        bool init(bool Server = true);
        void set_Async(bool flag);
        bool setSSLCertificate(const CTL::String &CertPath) const;
        bool setSSLKey(const CTL::String &KeyPath, const CTL::String &Passwd) const;
        bool setSSLVerify(const CTL::String &CA_Path) const;
        bool setSSLHostName(const CTL::String &hostName) const;  // 客户端 SNI
        bool isCTX() const;
        SSL_Client* createClient(const CTL::Socket* socket) const;
        SSL_Client* connectClient(const CTL::Socket* socket) const;
        SSL_CTX* getSSL_CTX() const;
        bool isServer() const { return m_isServer; }
    };
    class InitSSL_t {
    public:
        InitSSL_t();
        ~InitSSL_t();
    };
    // // 方式一：精细控制
    // CTL::CertManager::CertInfo_t info;
    // info.commonName = "api.example.com";
    // info.ips = {"10.0.0.1"};
    // info.days = 365;
    // info.keySize = 4096;
    // CTL::CertManager::GenerateSelfSignedCert("server.crt", "server.key", info);
    //
    // // 方式二：HTTPS 一站式
    // CTL::CertManager::GenerateHTTPSCert("./certs", "localhost");
    class CertManager {
    public:
        //----------------------------------------------------------------------------
        // 证书信息结构
        struct CertInfo_t {
            std::string country = "CN";              // 国家
            std::string state = "Guangdong";         // 省份
            std::string locality = "Shenzhen";       // 城市
            std::string organization = "XunBangKeJi";    // 组织
            std::string unit = "IT Department";      // 部门
            std::string commonName = "localhost";    // 域名/IP
            std::vector<std::string> ips;             // IP SAN 列表（如 {"127.0.0.1", "::1"}）
            int days = 365;                          // 有效期（天）
            int keySize = 2048;                      // RSA 密钥长度
        };
        static CTL::String AvaName;
        //----------------------------------------------------------------------------
        int Status; // 状态
        int InUse;  // 使用状态
        int Day;    // 持续时间
        CTL::String SerialNumber; // 证书序列号
        CTL::String Validity;     // 有效期
        CTL::String Issuer;       // 签发者
        CTL::String Name;         // 证书名称
        CTL::String IP;           // IP
        CTL::String OrganizationalUnit; // 组织单位
        CTL::String Organization;       //  组织
        CTL::String ValidityPeriod;     // 有效期
        CTL::String Region;             // 地区
        CTL::String Province;           // 省份
        CTL::String CityName;           // 城市
        //----------------------------------------------------------------------------
        /**
         * @brief 生成自签名 SSL 证书
	         * @param certPath 证书文件输出路径（.crt）
	         * @param keyPath 私钥文件输出路径（.key）
         * @param info 证书信息配置
         * @return 是否生成成功
         */
        static bool GenerateSelfSignedCert(const CTL::String& certPath,
                                           const CTL::String& keyPath,
                                           const CertInfo_t& info);
        /**
         * @brief 验证证书和私钥是否匹配
         * @param certPath 证书文件路径
         * @param keyPath 私钥文件路径
         * @return 是否匹配
         */
        static bool VerifyCertAndKey(const CTL::String& certPath, const CTL::String& keyPath);
        static CTL::String getSNumber(const CTL::String& certPath);
        /**
         * @brief 便捷方法：生成 HTTPS 自签名证书（输出到指定目录）
         * @param outputDir  输出目录，将生成 server.crt 和 server.key
         * @param commonName 域名或 IP（如 "example.com" 或 "192.168.1.1"）
         * @param ips        额外 IP SAN 列表
         * @param days       有效期（天），默认 3650（约10年）
         * @return 是否成功
         */
        static bool GenerateHTTPSCert(const CTL::String& outputDir,
                                      const CTL::String& commonName = "localhost",
                                      const std::vector<std::string>& ips = {},
                                      int days = 3650);
    private:
        static void PrintError(const CTL::String& msg);
        static void SetFilePermissions(const CTL::String& path, int mode);
    };
}


#endif
