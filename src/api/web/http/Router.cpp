 #include "api/web/http/Router.h"
#include "CCServlet/CCRequest.h"
#include "CCServlet/CCResponse.h"
#include <utility>

namespace api::web {

namespace {

// 把路径按 / 切成一段段："/api/camera/{id}/frame" -> [api, camera, {id}, frame]
std::vector<std::string> SplitSegments(const std::string& path) {
    std::vector<std::string> segments;
    size_t pos = 0;
    while (pos < path.size()) {
        if (path[pos] == '/') {
            ++pos;              // 跳过 /（连续多个 / 也一起跳过）
            continue;
        }
        size_t end = path.find('/', pos);
        if (end == std::string::npos) end = path.size();
        segments.push_back(path.substr(pos, end - pos));
        pos = end;
    }
    return segments;
}

} // namespace

void Router::Add(const std::string& method, const std::string& pattern, Handler handler) {
    RouteEntry route;
    route.method = method;
    route.segments = SplitSegments(pattern);   // 注册时就切好段，匹配时逐段比对
    route.handler = std::move(handler);
    routes_.push_back(std::move(route));
}

bool Router::Route(CTL::Request& req, CTL::Response& res) {
    std::string method = req.RequestData.Method;
    std::string path = req.RequestData.Path;

    // 逐条路由尝试：先比方法，再比段数，最后逐段匹配
    for (auto& route : routes_) {
        if (route.method != method) continue;

        auto segments = SplitSegments(path);
        if (segments.size() != route.segments.size()) continue;

        PathParams params;
        bool matched = true;
        for (size_t i = 0; i < segments.size(); ++i) {
            const std::string& pattern = route.segments[i];
            if (pattern.size() >= 2 && pattern.front() == '{' && pattern.back() == '}') {
                // {id} 占位符：匹配任意内容，并把值抓进 params
                params[pattern.substr(1, pattern.size() - 2)] = segments[i];
            } else if (pattern != segments[i]) {
                // 字面量段必须完全相等
                matched = false;
                break;
            }
        }
        if (!matched) continue;

        route.handler(req, res, params);
        return true;
    }
    return false;   // 没有路由命中，由调用方回 404
}

} // namespace api::web
