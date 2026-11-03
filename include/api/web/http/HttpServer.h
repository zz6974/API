#pragma once
#include <string>
#include "CCServlet/CCWebServlet.h"
#include "api/common/Config.h"
#include "api/web/http/Router.h"

namespace api::web {

// SDK WebServlet 的薄封装：全部请求统一走 Router
class HttpServer {
public:
    void Init(const common::ServerConfig& config, Router& router);
    void Run();

private:
    CTL::WebServlet server_;
    Router* router_ = nullptr;
    std::string address_;
    int port_ = 0;
};

} // namespace api::web
