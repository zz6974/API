#pragma once
#include <memory>
#include "api/app/service/AuthService.h"
#include "api/web/http/BaseController.h"
#include "api/web/http/Router.h"

namespace api::web {

// 认证控制器：路由 /api/login
class AuthController : public BaseController {
public:
    explicit AuthController(std::shared_ptr<app::AuthService> service);

    void RegisterRoutes(Router& router);

private:
    void HandleLogin(CTL::Request& req, CTL::Response& res, const PathParams& params);

    std::shared_ptr<app::AuthService> service_;
};

} // namespace api::web
