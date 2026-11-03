#include "api/web/http/HttpServer.h"
#include "CCServlet/CCRequest.h"
#include "CCServlet/CCResponse.h"
#include "api/common/Logger.h"

namespace api::web {

void HttpServer::Init(const common::ServerConfig& config, Router& router) {
    router_ = &router;
    address_ = config.httpAddress;
    port_ = config.httpPort;

    server_.Init(address_.c_str(), port_);
    // 所有请求统一交给 Router；路由未命中回 404
    server_.SetWebServletFun([this](CTL::Request& req, CTL::Response& res) {
        if (!router_->Route(req, res)) {
            res.SetHeader("Content-Type", "application/json; charset=utf-8");
            res.Write(R"({"code":404,"msg":"未找到路由"})", 404);
        }
    });
}

void HttpServer::Run() {
    LOG_INFO("HTTP 服务启动在 %s:%d", address_.c_str(), port_);
    server_.Running();
}

} // namespace api::web
