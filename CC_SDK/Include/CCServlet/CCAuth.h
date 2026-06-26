#ifndef CC_AUTH_H
#define CC_AUTH_H
#pragma once

#include "CCRequest.h"
#include "CCResponse.h"
#include "CCCookie.h"
#include "CCMap.h"
#include "CCThread.h"
#include <functional>
#include <memory>
#include <shared_mutex>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace CTL {

// ==================== Auth 结果 ====================

/**
 * 认证结果
 */
struct AuthResult {
    bool   Success = false;       ///< 认证是否成功
    CTL::String UserID;           ///< 认证成功后的用户标识
    CTL::String Message;          ///< 附加消息（失败原因等）
    CTL::String Realm = "Restricted Area"; ///< 认证域

    static AuthResult OK(const CTL::String& userId = "") {
        AuthResult r;
        r.Success = true;
        r.UserID  = userId;
        return r;
    }

    static AuthResult Fail(const CTL::String& message = "Authentication failed") {
        AuthResult r;
        r.Success = false;
        r.Message = message;
        return r;
    }
};

// ==================== 认证提供者基类 ====================

/**
 * 认证提供者接口
 * 所有认证方式（Basic、Bearer、Session 等）都实现此接口
 */
class IAuthProvider {
public:
    virtual ~IAuthProvider() = default;

    /**
     * 对请求进行认证
     * @param request 请求对象
     * @return AuthResult 认证结果
     */
    virtual AuthResult Authenticate(CTL::Request& request) = 0;

    /**
     * 认证失败时的响应处理
     * @param response 响应对象
     * @param result   认证失败结果
     */
    virtual void OnAuthFailed(CTL::Response& response, const AuthResult& result) = 0;

    /**
     * 获取认证方案名称
     */
    [[nodiscard]] virtual CTL::String GetSchemeName() const = 0;
};

// ==================== Basic Auth 提供者 ====================

/**
 * HTTP Basic Authentication 提供者
 */
class BasicAuthProvider : public IAuthProvider {
public:
    using CredentialValidator = std::function<bool(const CTL::String& username,
                                                    const CTL::String& password,
                                                    CTL::String& outUserId)>;

    /**
     * @param validator 凭据验证回调（返回 true 表示通过，outUserId 填入用户标识）
     * @param realm     认证域描述
     */
    explicit BasicAuthProvider(CredentialValidator validator,
                               const CTL::String& realm = "Restricted Area")
        : m_validator(std::move(validator)), m_realm(realm) {}

    AuthResult Authenticate(CTL::Request& request) override {
        CTL::String username, password;
        if (!request.GetBasicAuthCredentials(username, password)) {
            return AuthResult::Fail("Missing or invalid Basic Authorization header");
        }

        CTL::String userId;
        if (m_validator && m_validator(username, password, userId)) {
            AuthResult r = AuthResult::OK(userId.empty() ? username : userId);
            r.Realm = m_realm;
            return r;
        }
        return AuthResult::Fail("Invalid username or password");
    }

    void OnAuthFailed(CTL::Response& response, const AuthResult& result) override {
        response.RequireBasicAuth(result.Realm.empty() ? m_realm : result.Realm);
    }

    [[nodiscard]] CTL::String GetSchemeName() const override { return "Basic"; }

private:
    CredentialValidator m_validator;
    CTL::String m_realm;
};

// ==================== Bearer Token 提供者 ====================

/**
 * Bearer Token 认证提供者（JWT 或自定义 Token）
 */
class BearerTokenAuthProvider : public IAuthProvider {
public:
    using TokenValidator = std::function<bool(const CTL::String& token,
                                               CTL::String& outUserId)>;

    /**
     * @param validator Token 验证回调（返回 true 表示通过，outUserId 填入用户标识）
     * @param realm     认证域描述
     */
    explicit BearerTokenAuthProvider(TokenValidator validator,
                                     const CTL::String& realm = "Restricted Area")
        : m_validator(std::move(validator)), m_realm(realm) {}

    AuthResult Authenticate(CTL::Request& request) override {
        const CTL::String token = request.GetBearerToken();
        if (token.empty()) {
            return AuthResult::Fail("Missing Bearer token");
        }

        CTL::String userId;
        if (m_validator && m_validator(token, userId)) {
            AuthResult r = AuthResult::OK(userId);
            r.Realm = m_realm;
            return r;
        }
        return AuthResult::Fail("Invalid or expired token");
    }

    void OnAuthFailed(CTL::Response& response, const AuthResult& result) override {
        response.RequireBearerAuth(result.Realm.empty() ? m_realm : result.Realm);
    }

    [[nodiscard]] CTL::String GetSchemeName() const override { return "Bearer"; }

private:
    TokenValidator m_validator;
    CTL::String m_realm;
};

// ==================== Session Auth 提供者 ====================

/**
 * Session 数据结构
 */
struct SessionData {
    CTL::String SessionID;     ///< 会话 ID
    CTL::String UserID;        ///< 用户标识
    CTL::String Data;          ///< 附加数据（JSON 等）
    time_t      CreatedAt;     ///< 创建时间
    time_t      ExpiresAt;     ///< 过期时间

    [[nodiscard]] bool IsExpired() const {
        return ExpiresAt > 0 && std::time(nullptr) > ExpiresAt;
    }
};

/**
 * 简单的内存 Session 存储
 * 生产环境建议替换为 Redis 或数据库
 */
class SessionStore {
public:
    /**
     * 创建新 Session
     * @param userId       用户标识
     * @param ttlSeconds   存活秒数（默认 3600 = 1 小时）
     * @param data         附加数据
     * @return Session 数据（含生成的 SessionID）
     */
    SessionData CreateSession(const CTL::String& userId,
                              uint32_t ttlSeconds = 3600,
                              const CTL::String& data = "") {
        const CTL::String sessionId = GenerateSessionID();
        SessionData session;
        session.SessionID = sessionId;
        session.UserID    = userId;
        session.Data      = data;
        session.CreatedAt = std::time(nullptr);
        session.ExpiresAt = session.CreatedAt + ttlSeconds;

        std::unique_lock<std::mutex> lock(m_mutex);
        m_sessions[sessionId] = session;
        return session;
    }

    /**
     * 根据 SessionID 获取 Session
     * @param sessionId 会话 ID
     * @return SessionData 指针，不存在或过期则返回 nullptr
     */
    SessionData* GetSession(const CTL::String& sessionId) {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(sessionId);
        if (it == m_sessions.end()) return nullptr;
        if (it->second.IsExpired()) {
            lock.unlock();
            RemoveSession(sessionId);
            return nullptr;
        }
        return &it->second;
    }

    /**
     * 刷新 Session 过期时间
     * @param sessionId  会话 ID
     * @param ttlSeconds 新的存活秒数
     * @return 是否成功
     */
    bool RefreshSession(const CTL::String& sessionId, uint32_t ttlSeconds) {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(sessionId);
        if (it == m_sessions.end()) return false;
        it->second.ExpiresAt = std::time(nullptr) + ttlSeconds;
        return true;
    }

    /**
     * 删除 Session
     * @param sessionId 会话 ID
     */
    void RemoveSession(const CTL::String& sessionId) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_sessions.erase(sessionId);
    }

    /**
     * 清理所有过期 Session（建议定时调用）
     */
    void CleanupExpired() {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto it = m_sessions.begin();
        while (it != m_sessions.end()) {
            if (it->second.IsExpired()) {
                it = m_sessions.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * 获取当前 Session 数量
     */
    [[nodiscard]] size_t Count() const {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_sessions.size();
    }

private:
    static CTL::String GenerateSessionID() {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<uint64_t> dis;

        const auto now = std::chrono::high_resolution_clock::now();
        const auto nanos = std::chrono::time_point_cast<std::chrono::nanoseconds>(now)
                               .time_since_epoch().count();

        std::stringstream ss;
        ss << std::hex << std::setfill('0')
           << std::setw(16) << (nanos & 0xFFFFFFFFFFFFFFFF)
           << std::setw(16) << dis(gen);
        return CTL::String(ss.str());
    }

    mutable std::mutex m_mutex;
    CCMap<CTL::String, SessionData> m_sessions;
};

/**
 * 基于 Cookie + Session 的认证提供者
 */
class SessionAuthProvider : public IAuthProvider {
public:
    /**
     * @param store       Session 存储实例（共享指针，允许多个 provider 共用）
     * @param cookieName  Cookie 中存储 SessionID 的键名
     * @param realm       认证域描述
     */
    explicit SessionAuthProvider(std::shared_ptr<SessionStore> store,
                                 const CTL::String& cookieName = "CC_SESSION",
                                 const CTL::String& realm = "Session Auth")
        : m_store(std::move(store)), m_cookieName(cookieName), m_realm(realm) {}

    AuthResult Authenticate(CTL::Request& request) override {
        // 从 Cookie 中获取 SessionID
        request.ParseCookies();
        const CTL::String sessionId = request.GetCookie(m_cookieName);
        if (sessionId.empty()) {
            return AuthResult::Fail("No session cookie found");
        }

        if (!m_store) {
            return AuthResult::Fail("Session store not available");
        }

        SessionData* session = m_store->GetSession(sessionId);
        if (!session) {
            return AuthResult::Fail("Invalid or expired session");
        }

        AuthResult r = AuthResult::OK(session->UserID);
        r.Realm = m_realm;
        return r;
    }

    void OnAuthFailed(CTL::Response& response, const AuthResult& /*result*/) override {
        // Session 过期/无效时，清除客户端 Cookie
        response.RemoveCookie(m_cookieName);
        response.SendUnauthorized("Session invalid or expired");
    }

    [[nodiscard]] CTL::String GetSchemeName() const override { return "Session"; }

    /**
     * 登录后创建 Session 并设置 Cookie
     * @param response    响应对象
     * @param userId      用户标识
     * @param ttlSeconds  Session 存活秒数
     * @param data        附加数据
     * @return 创建的 SessionData
     */
    SessionData Login(CTL::Response& response, const CTL::String& userId,
                      uint32_t ttlSeconds = 3600, const CTL::String& data = "") {
        if (!m_store) return {};

        const SessionData session = m_store->CreateSession(userId, ttlSeconds, data);

        // 设置 Cookie
        Cookie cookie = Cookie::SecureToken(m_cookieName, session.SessionID, ttlSeconds);
        cookie.Path = "/";
        response.SetCookie(cookie);
        response.SetHeader("Connection","close");
        return session;
    }

    /**
     * 登出：删除 Session 并清除 Cookie
     * @param request  请求对象（用于提取 SessionID）
     * @param response 响应对象
     */
    void Logout(CTL::Request& request, CTL::Response& response) {
        request.ParseCookies();
        const CTL::String sessionId = request.GetCookie(m_cookieName);
        if (!sessionId.empty() && m_store) {
            m_store->RemoveSession(sessionId);
        }
        response.RemoveCookie(m_cookieName);
    }

    /**
     * 获取 SessionStore 指针
     */
    std::shared_ptr<SessionStore> GetStore() const { return m_store; }

    /**
     * 获取 Cookie 名称
     */
    [[nodiscard]] CTL::String GetCookieName() const { return m_cookieName; }

private:
    std::shared_ptr<SessionStore> m_store;
    CTL::String m_cookieName;
    CTL::String m_realm;
};

// ==================== 组合认证提供者 ====================

/**
 * 组合多个认证提供者，按顺序尝试，任一成功即认证通过
 */
class CompositeAuthProvider : public IAuthProvider {
public:
    CompositeAuthProvider() = default;

    /**
     * 添加一个认证提供者
     */
    void AddProvider(std::shared_ptr<IAuthProvider> provider) {
        m_providers.push_back(std::move(provider));
    }

    AuthResult Authenticate(CTL::Request& request) override {
        AuthResult lastFail;
        for (auto& provider : m_providers) {
            AuthResult r = provider->Authenticate(request);
            if (r.Success) return r;
            lastFail = r;
        }
        return lastFail.Success ? lastFail
                                : AuthResult::Fail("All authentication methods failed");
    }

    void OnAuthFailed(CTL::Response& response, const AuthResult& result) override {
        // 默认使用第一个 provider 的失败处理
        if (!m_providers.empty()) {
            m_providers.front()->OnAuthFailed(response, result);
        } else {
            response.SendUnauthorized(result.Message);
        }
    }

    [[nodiscard]] CTL::String GetSchemeName() const override { return "Composite"; }

private:
    CCVector<std::shared_ptr<IAuthProvider>> m_providers;
};

// ==================== Auth 中间件 ====================

/**
 * 认证中间件：包装一个请求处理函数，在执行前进行认证检查
 *
 * 使用示例:
 * @code
 *   auto auth = std::make_shared<BasicAuthProvider>([](auto& u, auto& p, auto& id) {
 *       return u == "admin" && p == "123456";
 *   });
 *   servlet.SetWebServlet("/api/secret", AuthMiddleware(auth, [](Request& req, Response& resp) {
 *       resp.Write("Welcome, authenticated user!");
 *   }));
 * @endcode
 */
class AuthMiddleware {
public:
    /**
     * @param provider 认证提供者
     * @param next     认证通过后执行的处理函数
     */
    AuthMiddleware(std::shared_ptr<IAuthProvider> provider,
                   std::function<void(Request&, Response&)> next)
        : m_provider(std::move(provider)), m_next(std::move(next)) {}

    /**
     * 执行认证 + 业务逻辑
     */
    void operator()(CTL::Request& request, CTL::Response& response) const {
        if (!m_provider) {
            if (m_next) m_next(request, response);
            return;
        }

        const AuthResult result = m_provider->Authenticate(request);
        if (result.Success) {
            if (m_next) m_next(request, response);
        } else {
            m_provider->OnAuthFailed(response, result);
        }
    }

    /**
     * 获取包装后的 RequestFunc，可直接传给 SetWebServlet
     * 安全捕获：按值捕获 m_provider 和 m_next，避免 this 悬垂
     */
    [[nodiscard]] std::function<void(Request&, Response&)> GetHandler() const {
        auto provider = m_provider;  // shared_ptr 拷贝，引用计数+1
        auto next     = m_next;      // std::function 拷贝
        return [provider, next](CTL::Request& req, CTL::Response& resp) {
            if (!provider) {
                if (next) next(req, resp);
                return;
            }
            const AuthResult result = provider->Authenticate(req);
            if (result.Success) {
                if (next) next(req, resp);
            } else {
                provider->OnAuthFailed(resp, result);
            }
        };
    }

private:
    std::shared_ptr<IAuthProvider> m_provider;
    std::function<void(Request&, Response&)> m_next;
};

} // namespace CTL

#endif // CC_AUTH_H
