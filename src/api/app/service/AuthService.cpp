#include "api/app/service/AuthService.h"
#include <utility>

namespace api::app {

AuthService::AuthService(std::string account, std::string password)
    : account_(std::move(account)), password_(std::move(password)) {
}

common::Result<void> AuthService::Authenticate(const std::string& account,
                                               const std::string& password) const {
    if (account != account_ || password != password_) {
        return common::Result<void>::Fail(common::ErrorCode::kUnauthorized, "账号或密码错误");
    }
    return common::Result<void>::Ok();
}

} // namespace api::app
