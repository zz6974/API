#include "api/web/http/Router.h"
#include "CCServlet/CCRequest.h"
#include "CCServlet/CCResponse.h"
#include <utility>

namespace api::web {

namespace {

std::vector<std::string> SplitSegments(const std::string& path) {
    std::vector<std::string> segments;
    size_t pos = 0;
    while (pos < path.size()) {
        if (path[pos] == '/') {
            ++pos;
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
    route.segments = SplitSegments(pattern);
    route.handler = std::move(handler);
    routes_.push_back(std::move(route));
}

bool Router::Route(CTL::Request& req, CTL::Response& res) {
    std::string method = req.RequestData.Method;
    std::string path = req.RequestData.Path;

    for (auto& route : routes_) {
        if (route.method != method) continue;

        auto segments = SplitSegments(path);
        if (segments.size() != route.segments.size()) continue;

        PathParams params;
        bool matched = true;
        for (size_t i = 0; i < segments.size(); ++i) {
            const std::string& pattern = route.segments[i];
            if (pattern.size() >= 2 && pattern.front() == '{' && pattern.back() == '}') {
                params[pattern.substr(1, pattern.size() - 2)] = segments[i];
            } else if (pattern != segments[i]) {
                matched = false;
                break;
            }
        }
        if (!matched) continue;

        route.handler(req, res, params);
        return true;
    }
    return false;
}

} // namespace api::web
