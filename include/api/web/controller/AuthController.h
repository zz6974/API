#pragma once
#include <memory>
#include "api/app/service/AuthService.h"
#include "api/web/http/BaseController.h"
#include "api/web/http/Router.h"

namespace api::web {

/**
 * @brief 认证控制器
 *
 * 路由 /api/login。
 */
class AuthController : public BaseController {
public:
    /**
     * @brief 构造
     * @param service 认证用例服务
     */
    explicit AuthController(std::shared_ptr<app::AuthService> service);

    /**
     * @brief 注册路由
     * @param router 路由表
     */
    void RegisterRoutes(Router& router); // 注册 1 条路由：登录

private:
    void HandleLogin(CTL::Request& req, CTL::Response& res, const PathParams& params); ///< 校验 POST 体里的 Account / Password
    std::shared_ptr<app::AuthService> service_; ///< 认证用例服务
};

} // namespace api::web
