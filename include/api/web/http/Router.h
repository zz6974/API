#pragma once
#include <string>
#include <vector>
#include "api/web/http/HttpTypes.h"

namespace api::web {

// 支持 {id} 占位符的路由表，替代旧 Analysis/URL 的手工路径解析
class Router {
public:
    // pattern 示例: "POST /api/camera/{id}/frame"
    void Add(const std::string& method, const std::string& pattern, Handler handler);

    // 匹配失败返回 false（调用方回 404）
    bool Route(CTL::Request& req, CTL::Response& res);

private:
    struct RouteEntry {
        std::string method;
        std::vector<std::string> segments;  // 字面量段，{id} 处存参数名
        Handler handler;
    };

    std::vector<RouteEntry> routes_;
};

} // namespace api::web
