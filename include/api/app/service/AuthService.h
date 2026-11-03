#pragma once
#include <string>
#include "api/common/Result.h"

namespace api::app {

// 认证用例服务：账号密码来自配置，后续可替换为数据库实现
class AuthService {
public:
    AuthService(std::string account, std::string password);

    common::Result<void> Authenticate(const std::string& account,
                                      const std::string& password) const;

private:
    std::string account_;
    std::string password_;
};

} // namespace api::app
