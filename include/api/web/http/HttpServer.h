#pragma once
#include <string>
#include "CCServlet/CCWebServlet.h"
#include "api/common/Config.h"
#include "api/web/http/Router.h"

namespace api::web {

/**
 * @brief SDK WebServlet 的薄封装
 *
 * 全部请求统一走 Router。
 */
class HttpServer {
public:
    /**
     * @brief 初始化 SDK 服务器并挂载路由分发回调
     * @param config 服务配置（取监听地址与端口）
     * @param router 路由表
     */
    void Init(const common::ServerConfig& config, Router& router);
    /**
     * @brief 阻塞运行 HTTP 服务
     *
     * 主线程调用后不再返回。
     */
    void Run();

private:
    CTL::WebServlet server_;
    Router* router_ = nullptr;   ///< 不拥有 Router，生命周期由 AppContext 管理
    std::string address_;        ///< 监听地址
    int port_ = 0;               ///< 监听端口
};

} // namespace api::web
