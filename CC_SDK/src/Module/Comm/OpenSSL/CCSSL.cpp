#include "OpenSSL/CCSSL.h"

#include <openssl/x509v3.h>

#include "err.h"
#include "CCSystem.h"
#include "CCThread.h"
#include "CCFile.h"

extern "C"{
#if defined(_WIN32) || defined(_WIN64)
#include "openssl/applink.h"
#elif defined(__linux__)

#endif
}

#if defined(_WIN32) || defined(_WIN64)

#elif defined(__linux__)
#include "sys/stat.h"
#endif

CTL::String CTL::CertManager::AvaName = "Default";

CTL::SSL_Client::SSL_Client(ssl_st *ssl) {
    this->ssl = ssl;
}

CTL::SSL_Client::~SSL_Client() {
    if (ssl) {
        const bool F = !IsShutDown(ssl);
        if (F) {
            SSL_shutdown(ssl);
            SSL_certs_clear(ssl);
            SSL_free(ssl);
            // System::Println("清理SSL成功");
        }
        ssl = nullptr;
    }
}

ByteHander CTL::SSL_Client::read(char *buffer, const size_t size) const {
    // size_t bytes_read;
    int result = SSL_read(ssl, buffer, size);
    if (result > 0) {
        return result;
    }
    const int ssl_error = SSL_get_error(ssl, result);
    if (ssl_error == SSL_ERROR_WANT_READ) {
        result = -2;
    }
    else if (ssl_error == SSL_ERROR_SYSCALL) {
        result = -5;
    }
    else if (ssl_error == SSL_ERROR_SSL) {
        result = -1;
    }
    return result;
}

ByteHander CTL::SSL_Client::read_ex(char* buffer, size_t size) const {
    size_t bytes_read = 0;
    int result = SSL_read_ex(ssl, buffer, size,&bytes_read);
    if (result > 0) {
        return bytes_read;
    }
    const int ssl_error = SSL_get_error(ssl, result);
    if (ssl_error == SSL_ERROR_WANT_READ) {
        result = -2;
    }
    else if (ssl_error == SSL_ERROR_SYSCALL) {
        result = -5;
    }
    else if (ssl_error == SSL_ERROR_SSL) {
        result = -1;
    }
    if (bytes_read > 0) {
        return bytes_read;
    }
    return result;
}

int CTL::SSL_Client::write(const char *buffer, const size_t size) const {
    if (ssl == nullptr) {
        return -1; // 错误ssl
    }
    // 检查连接状态
    if (SSL_get_shutdown(ssl) & (SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN)) {
        return -4; // 连接已关闭
    }
    // 检查握手状态
    if (!SSL_is_init_finished(ssl)) {
        return -8; // 握手未完成
    }

    size_t bytes_written;
    int result = SSL_write_ex(ssl, buffer, size, &bytes_written);
    if (result != 1) {
        const int err = SSL_get_error(ssl, result);
        switch (err) {
            case SSL_ERROR_ZERO_RETURN: {
                // The other party has closed the connection. This is a normal situation
                return 0;
            }
            case SSL_ERROR_WANT_WRITE: {
                // Non-blocking operations require waiting for writable events
                return -5;
            }
            case SSL_ERROR_SYSCALL: {
                // System call errors, which may include SIGPIPE
                if (ERR_peek_error() == 0) {
                    // If there are no SSL errors, check for system errors
                    if (errno == EPIPE || errno == ECONNRESET) {
                        // The connection has been disconnected or the pipe has burst
                        return -4;
                    }
                }
                // Other system errors
                return -3;
            }
            case SSL_ERROR_SSL: {
                // SSL protocol error
                ERR_print_errors_fp(stderr);
                return -2;
            }
            default: {
                // Other SSL errors
                return 99;
            }
        }
    }
    // Check whether the number of bytes written matches the expected value
    if (bytes_written != size) {
        // The write is incomplete and may require retry or error handling
        return 98;
    }
    return bytes_written;
}

SSL * CTL::SSL_Client::getSSL() const {
    return this->ssl;
}

bool CTL::SSL_Client::IsShutDown(const ssl_st *ssl) {
    if (ssl) {
        return (SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN);
    }
    return true;
}

void CTL::SSL_Client::release() {
    ssl = nullptr;
}

bool CTL::SSL_Client::available() const {
    if (ssl) {
        // 首先检查SSL层是否有数据
        if (SSL_pending(ssl) > 0) {
            return true;
        }
        // 检查底层socket是否有数据
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

CTL::SSL_BIO::SSL_BIO(BIO *r_bio, BIO *w_bio) {
    this->r_bio = r_bio;
    this->w_bio = w_bio;
}

CTL::SSL_BIO::~SSL_BIO() {
    if (r_bio) {
        BIO_free(r_bio);
        r_bio = nullptr;
    }
    if (w_bio) {
        BIO_free(w_bio);
        w_bio = nullptr;
    }
}

CTL::CTX_SSL::CTX_SSL() {
    ssl_ctx = nullptr;
}

CTL::CTX_SSL::CTX_SSL(const bool Server) {
    init(Server);
}

CTL::CTX_SSL::~CTX_SSL() {
    // 清理 SSL 上下文
    if (this->ssl_ctx) {
        SSL_CTX_free(this->ssl_ctx);
        ssl_ctx = nullptr;
    }
    // 清理 OpenSSL 库
    // CONF_modules_unload(1);
    EVP_cleanup();
    ERR_free_strings();
    CRYPTO_cleanup_all_ex_data();
}

bool CTL::CTX_SSL::init(const bool Server) {
#ifdef _WIN32
    OPENSSL_Applink();
#elif __linux__
    struct sigaction sa{};
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, nullptr);
#endif
    m_isServer = Server;

    // OpenSSL 基础初始化
    OPENSSL_init_ssl(
        OPENSSL_INIT_LOAD_SSL_STRINGS |
        OPENSSL_INIT_LOAD_CRYPTO_STRINGS,
        nullptr
    );
    SSL_load_error_strings();

    // 根据角色选择 METHOD
    const SSL_METHOD* meth = Server ? TLS_server_method() : TLS_client_method();
    ssl_ctx = SSL_CTX_new(meth);
    if (!ssl_ctx) {
        ERR_print_errors_fp(stderr);
        return false;
    }

    // 通用 SSL 选项
    SSL_CTX_set_options(ssl_ctx,
           SSL_OP_NO_SSLv2 |
           SSL_OP_NO_SSLv3 |
           SSL_OP_NO_COMPRESSION
    );

    if (Server) {
        // --- 服务端配置 ---
        SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_3_VERSION);
        SSL_CTX_set_max_proto_version(ssl_ctx, TLS1_3_VERSION);
        SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_SERVER);
        SSL_CTX_sess_set_cache_size(ssl_ctx, 1024);
        SSL_CTX_set_timeout(ssl_ctx, 300);
        // 不校验客户端证书（服务端通常不验证客户端）
        SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, nullptr);
    }
    else {
        // --- 客户端配置 ---
        // 客户端需要验证服务端证书
        SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, nullptr);
        // 客户端允许更广泛的 TLS 版本（兼容更多服务端）
        SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION);
    }

    return true;
}

void CTL::CTX_SSL::set_Async(const bool flag) {
    this->is_Async = flag;
}

bool CTL::CTX_SSL::setSSLCertificate(const CTL::String &CertPath) const {
    if (SSL_CTX_use_certificate_file(ssl_ctx, CertPath.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }
    return true;
}

bool CTL::CTX_SSL::setSSLKey(const CTL::String &KeyPath, const CTL::String &Passwd) const {
    void* passwd = nullptr;
    if(!Passwd.empty()) {
        passwd = (void*)Passwd.c_str();
    }
    if(passwd != nullptr)
        SSL_CTX_set_default_passwd_cb_userdata(ssl_ctx,passwd);
    if (SSL_CTX_use_PrivateKey_file(ssl_ctx, KeyPath.c_str(), SSL_FILETYPE_PEM) != 1) {
        ERR_print_errors_fp(stderr);
        return false;
    }
    if(!SSL_CTX_check_private_key(ssl_ctx)) {
        CTL::System::Println("setSSLKey SSL_CTX_check_private_key Error");
        return false;
    }
    return true;
}

bool CTL::CTX_SSL::setSSLVerify(const CTL::String &CA_Path) const {
    if(!SSL_CTX_load_verify_locations(ssl_ctx, CA_Path.c_str(), nullptr)) {
        CTL::System::Println("setSSLVerify SSL_CTX_load_verify_locations Error");
        return false;
    }
    return true;
}

bool CTL::CTX_SSL::setSSLHostName(const CTL::String &hostName) const {
    if (!ssl_ctx) return false;
    // 存储 SNI 主机名，在实际 connect 时应用
    const_cast<CTX_SSL*>(this)->m_hostName = hostName;
    return true;
}

bool CTL::CTX_SSL::isCTX() const {
    return this->ssl_ctx != nullptr;
}

CTL::SSL_Client* CTL::CTX_SSL::createClient(const CTL::Socket *socket) const {
    if (this->ssl_ctx == nullptr) {
        return nullptr;
    }
    if (!socket) {
        return nullptr;
    }
    if (!m_isServer) {
        CTL::System::Println("createClient: CTX_SSL is in client mode, use connectClient() instead");
        return nullptr;
    }

    SSL* ssl = SSL_new(this->ssl_ctx);
    if (!ssl) {
        ERR_print_errors_fp(stderr);
        return nullptr;
    }
    SSL_set_accept_state(ssl);
    SSL_set_fd(ssl, socket->Socketbit);
    int ret;
    int retryTimes = 0;
    while ((ret = SSL_accept(ssl)) <= 0 && retryTimes++ < 50) {
        const int err = SSL_get_error(ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            CTL::Thread::Sleep(100);
            ret = 0;
            continue;
        }
        else if (err == SSL_ERROR_SSL) {
            SSL_free(ssl);
            break;
        }
        else {
            SSL_free(ssl);
            return nullptr;
        }
    }
    return (ret > 0) ? new SSL_Client(ssl) : nullptr;
}

CTL::SSL_Client* CTL::CTX_SSL::connectClient(const CTL::Socket *socket) const {
    if (this->ssl_ctx == nullptr) {
        return nullptr;
    }
    if (!socket) {
        return nullptr;
    }
    if (m_isServer) {
        CTL::System::Println("connectClient: CTX_SSL is in server mode, use createClient() instead");
        return nullptr;
    }

    SSL* ssl = SSL_new(this->ssl_ctx);
    if (!ssl) {
        ERR_print_errors_fp(stderr);
        return nullptr;
    }
    SSL_set_connect_state(ssl);
    SSL_set_fd(ssl, socket->Socketbit);

    // 设置 SNI（Server Name Indication）
    if (!m_hostName.empty()) {
        SSL_set_tlsext_host_name(ssl, m_hostName.c_str());
    }

    int ret;
    int retryTimes = 0;
    while ((ret = SSL_connect(ssl)) <= 0 && retryTimes++ < 50) {
        const int err = SSL_get_error(ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            CTL::Thread::Sleep(100);
            ret = 0;
            continue;
        }
        else if (err == SSL_ERROR_SSL) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            return nullptr;
        }
        else {
            SSL_free(ssl);
            return nullptr;
        }
    }
    return (ret > 0) ? new SSL_Client(ssl) : nullptr;
}

SSL_CTX * CTL::CTX_SSL::getSSL_CTX() const {
    return this->ssl_ctx;
}

CTL::InitSSL_t::InitSSL_t() {
#ifdef _WIN32
    OPENSSL_Applink();
#elif __linux__
    struct sigaction sa{};
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, nullptr);
#endif
}

CTL::InitSSL_t::~InitSSL_t() = default;

bool CTL::CertManager::GenerateSelfSignedCert(const CTL::String& certPath, const CTL::String& keyPath,
                                                  const CertInfo_t& info) {
    // 1. 初始化 OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    EVP_PKEY* pkey = nullptr;
    X509* x509 = nullptr;
    BIO* keyBio = nullptr;
    BIO* certBio = nullptr;

    // 2. 使用 EVP 通用 API 生成 RSA 密钥（兼容 OpenSSL 1.1 / 3.0）
    {
        EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
        if (!pctx || EVP_PKEY_keygen_init(pctx) <= 0 ||
            EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, info.keySize) <= 0 ||
            EVP_PKEY_keygen(pctx, &pkey) <= 0) {
            EVP_PKEY_CTX_free(pctx);
            PrintError("Failed to generate RSA key via EVP_PKEY_keygen");
            BIO_free(keyBio);
            BIO_free(certBio);
            X509_free(x509);
            EVP_PKEY_free(pkey);
            return false;
        }
        EVP_PKEY_CTX_free(pctx);
    }

    // 3. 创建 X509 证书
    x509 = X509_new();
    if (!x509) {
        PrintError("Failed to create X509 certificate");
        BIO_free(keyBio);
        BIO_free(certBio);
        X509_free(x509);
        EVP_PKEY_free(pkey);
        return false;
    }

    // 设置版本号（X509 v3）
    X509_set_version(x509, 2);

    // 设置序列号（使用时间戳作为随机种子）
    ASN1_INTEGER_set(X509_get_serialNumber(x509), static_cast<long>(time(nullptr)));

    // 设置有效期
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), static_cast<long>(info.days) * 24 * 60 * 60);

    // 设置公钥
    X509_set_pubkey(x509, pkey);

    // 4. 设置证书主题信息
    // Windows 下 subject 是宏，需重命名为 pSubjectName
#ifdef _WIN32
    auto pSubjectName = X509_get_subject_name(x509);
#else
    X509_NAME* subject = X509_get_subject_name(x509);
#endif

#define ADD_NAME_ENTRY(name, oid, field) \
    X509_NAME_add_entry_by_txt(name, oid, MBSTRING_ASC, \
                               reinterpret_cast<const unsigned char*>(info.field.c_str()), -1, -1, 0)

#ifdef _WIN32
    ADD_NAME_ENTRY(pSubjectName, "C", country);
    ADD_NAME_ENTRY(pSubjectName, "ST", state);
    ADD_NAME_ENTRY(pSubjectName, "L", locality);
    ADD_NAME_ENTRY(pSubjectName, "O", organization);
    ADD_NAME_ENTRY(pSubjectName, "OU", unit);
    ADD_NAME_ENTRY(pSubjectName, "CN", commonName);
    X509_set_issuer_name(x509, pSubjectName);
#else
    ADD_NAME_ENTRY(subject, "C", country);
    ADD_NAME_ENTRY(subject, "ST", state);
    ADD_NAME_ENTRY(subject, "L", locality);
    ADD_NAME_ENTRY(subject, "O", organization);
    ADD_NAME_ENTRY(subject, "OU", unit);
    ADD_NAME_ENTRY(subject, "CN", commonName);
    X509_set_issuer_name(x509, subject);
#endif

#undef ADD_NAME_ENTRY

    // 5. 添加 X509 v3 扩展
    X509V3_CTX ctx;
    X509V3_set_ctx_nodb(&ctx);
    X509V3_set_ctx(&ctx, x509, x509, nullptr, nullptr, 0);

    // 基本约束（CA:FALSE）
    {
        X509_EXTENSION* ext = X509V3_EXT_conf_nid(nullptr, &ctx, NID_basic_constraints, "CA:FALSE");
        if (ext) { X509_add_ext(x509, ext, -1); X509_EXTENSION_free(ext); }
    }

    // 主题备用名称（SAN）：DNS + IP
    {
        std::string san;
        if (!info.commonName.empty()) {
            san += "DNS:" + info.commonName;
        }
        for (const auto& ip : info.ips) {
            if (!san.empty()) san += ",";
            san += "IP:" + ip;
        }
        if (!san.empty()) {
            X509_EXTENSION* ext = X509V3_EXT_conf_nid(nullptr, &ctx, NID_subject_alt_name, san.c_str());
            if (ext) { X509_add_ext(x509, ext, -1); X509_EXTENSION_free(ext); }
        }
    }

    // 密钥用途
    {
        X509_EXTENSION* ext = X509V3_EXT_conf_nid(nullptr, &ctx, NID_key_usage,
                                                   "digitalSignature,keyEncipherment");
        if (ext) { X509_add_ext(x509, ext, -1); X509_EXTENSION_free(ext); }
    }

    // 扩展密钥用途（服务器+客户端认证）
    {
        X509_EXTENSION* ext = X509V3_EXT_conf_nid(nullptr, &ctx, NID_ext_key_usage,
                                                   "serverAuth,clientAuth");
        if (ext) { X509_add_ext(x509, ext, -1); X509_EXTENSION_free(ext); }
    }

    // 6. 自签名
    if (X509_sign(x509, pkey, EVP_sha256()) == 0) {
        PrintError("Failed to sign certificate");
        goto cleanup;
    }

    // 7. 保存私钥（PEM 格式）
    keyBio = BIO_new_file(keyPath.c_str(), "w");
    if (!keyBio) {
        PrintError(CTL::String::format("Failed to open key file: %s", keyPath.c_str()));
        goto cleanup;
    }
    if (PEM_write_bio_PrivateKey(keyBio, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        PrintError("Failed to write private key");
        goto cleanup;
    }

    // 8. 保存证书（PEM 格式）
    certBio = BIO_new_file(certPath.c_str(), "w");
    if (!certBio) {
        PrintError(CTL::String::format("Failed to open cert file: %s", certPath.c_str()));
        goto cleanup;
    }
    if (PEM_write_bio_X509(certBio, x509) != 1) {
        PrintError("Failed to write certificate");
        goto cleanup;
    }

    // 9. 设置文件权限
    SetFilePermissions(keyPath, 0600);
    SetFilePermissions(certPath, 0644);

    // 清理并返回成功
    BIO_free(keyBio);
    BIO_free(certBio);
    X509_free(x509);
    EVP_PKEY_free(pkey);
    return true;

cleanup:
    BIO_free(keyBio);
    BIO_free(certBio);
    X509_free(x509);
    EVP_PKEY_free(pkey);
    return false;
}

bool CTL::CertManager::VerifyCertAndKey(const CTL::String& certPath, const CTL::String& keyPath) {
    // 读取证书
    BIO* certBio = BIO_new_file(certPath.c_str(), "r");
    if (!certBio) {
        return false;
    }
    X509* cert = PEM_read_bio_X509(certBio, nullptr, nullptr, nullptr);
    BIO_free(certBio);
    if (!cert) {
        return false;
    }

    // 读取私钥
    BIO* keyBio = BIO_new_file(keyPath.c_str(), "r");
    if (!keyBio) {
        X509_free(cert);
        return false;
    }
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(keyBio, nullptr, nullptr, nullptr);
    BIO_free(keyBio);
    if (!pkey) {
        X509_free(cert);
        return false;
    }

    // 获取证书中的公钥
    EVP_PKEY* certPubKey = X509_get_pubkey(cert);
    if (!certPubKey) {
        X509_free(cert);
        EVP_PKEY_free(pkey);
        return false;
    }

    // 比较公钥和私钥是否匹配
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    // OpenSSL 3.0+ 使用 EVP_PKEY_eq
    bool match = (EVP_PKEY_eq(certPubKey, pkey) == 1);
#else
    // OpenSSL 1.x 使用 RSA 模数比较
    bool match = false;
    if (EVP_PKEY_id(certPubKey) == EVP_PKEY_RSA && EVP_PKEY_id(pkey) == EVP_PKEY_RSA) {
        RSA* certRsa = EVP_PKEY_get1_RSA(certPubKey);
        RSA* keyRsa = EVP_PKEY_get1_RSA(pkey);
        if (certRsa && keyRsa) {
            const BIGNUM* certN = RSA_get0_n(certRsa);
            const BIGNUM* keyN = RSA_get0_n(keyRsa);
            match = (BN_cmp(certN, keyN) == 0);
        }
        RSA_free(certRsa);
        RSA_free(keyRsa);
    }
#endif

    EVP_PKEY_free(certPubKey);
    EVP_PKEY_free(pkey);
    X509_free(cert);

    return match;
}

CTL::String CTL::CertManager::getSNumber(const CTL::String& certPath) {
    // 初始化 OpenSSL 错误队列
    ERR_clear_error();

    // 打开证书文件
    BIO* certBio = BIO_new_file(certPath.c_str(), "r");
    if (!certBio) {
        PrintError("Failed to open certificate file: " + certPath);
        return "";
    }

    // 读取 X509 证书
    X509* cert = PEM_read_bio_X509(certBio, nullptr, nullptr, nullptr);
    BIO_free(certBio);

    if (!cert) {
        PrintError("Failed to read X509 certificate from file");
        return "";
    }

    // 获取证书序列号
    const ASN1_INTEGER* serial = X509_get0_serialNumber(cert);
    if (!serial) {
        PrintError("Failed to get serial number from certificate");
        X509_free(cert);
        return "";
    }

    // 将序列号转换为十六进制字符串
    CTL::String serialHex;
    const int serialLen = ASN1_STRING_length(serial);
    const unsigned char* serialData = ASN1_STRING_get0_data(serial);

    if (serialLen > 0 && serialData) {
        char hexBuf[512] = {0};
        char* ptr = hexBuf;

        for (int i = 0; i < serialLen && ptr < hexBuf + sizeof(hexBuf) - 3; i++) {
            sprintf(ptr, "%02X", serialData[i]);
            ptr += 2;

            // 在每两个字节之间添加冒号分隔符
            if (i < serialLen - 1) {
                *ptr++ = ':';
            }
        }

        serialHex = hexBuf;
    } else {
        // 如果无法获取原始数据，则使用 ASN1_INTEGER 直接转换
        BIGNUM* bn = ASN1_INTEGER_to_BN(serial, nullptr);
        if (bn) {
            char* hex = BN_bn2hex(bn);
            if (hex) {
                serialHex = hex;
                OPENSSL_free(hex);
            }
            BN_free(bn);
        }
    }

    // 清理资源
    X509_free(cert);

    return serialHex;
}

void CTL::CertManager::PrintError(const CTL::String& msg) {
    char errorBuf[256] = {0};
    const unsigned long errCode = ERR_get_error();
    if (errCode != 0) {
        ERR_error_string_n(errCode, errorBuf, sizeof(errorBuf));
        CTL::System::Println(msg + " | OpenSSL: " + CTL::String(errorBuf));
    } else {
        CTL::System::Println(msg);
    }
}

void CTL::CertManager::SetFilePermissions(const CTL::String& path, const int mode) {
#ifdef __linux__
    chmod(path.c_str(), mode);
#endif
}

bool CTL::CertManager::GenerateHTTPSCert(const CTL::String& outputDir,
                                                  const CTL::String& commonName,
                                                  const std::vector<std::string>& ips,
                                                  const int days) {
    CertInfo_t info;
    info.commonName = commonName;
    info.ips = ips;
    info.days = days;
    info.keySize = 2048;

    // 确保输出目录存在
    if (!CTL::File::Create(outputDir, true) && !CTL::File(outputDir).isExists()) {
        PrintError("Failed to create output directory: " + outputDir);
        return false;
    }

    const CTL::String certPath = outputDir + "/server.crt";
    const CTL::String keyPath = outputDir + "/server.key";

    if (!GenerateSelfSignedCert(certPath, keyPath, info)) {
        return false;
    }

    // 验证生成的证书和私钥
    if (!VerifyCertAndKey(certPath, keyPath)) {
        PrintError("Generated certificate and key do not match!");
        return false;
    }

    CTL::System::Println("HTTPS certificate generated successfully:");
    CTL::System::Println("  Cert: " + certPath);
    CTL::System::Println("  Key:  " + keyPath);
    CTL::System::Println("  CN:   " + commonName);
    CTL::System::Println("  Days: " + CTL::String(std::to_string(days)));

    return true;
}

