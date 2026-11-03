#pragma once
#include <string>
#include "api/common/Result.h"

namespace api::app {

/**
 * @brief 认证用例服务
 *
 * 账号密码来自配置，后续可替换为数据库实现。
 */
class AuthService {
public:
    /**
     * @brief 构造
     * @param account 预期账号
     * @param password 预期密码
     */
    AuthService(std::string account, std::string password);

    /**
     * @brief 校验账号密码
     * @param account 待校验账号
     * @param password 待校验密码
     * @return 校验通过或失败；不匹配返回 kUnauthorized
     */
    common::Result<void> Authenticate(const std::string& account,
                                      const std::string& password) const;

private:
    std::string account_;  ///< 预期账号
    std::string password_; ///< 预期密码
};

} // namespace api::app
