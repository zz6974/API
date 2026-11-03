#pragma once
#include <string>
#include <vector>
#include "api/web/http/HttpTypes.h"

namespace api::web {

/**
 * @brief 支持 {id} 占位符的路由表
 *
 * 替代旧 Analysis/URL 的手工路径解析。
 */
class Router {
public:
    /**
     * @brief 注册一条路由
     * @param method HTTP 方法（GET / POST）
     * @param pattern 路径模式，如 "POST /api/camera/{id}/frame"
     * @param handler 命中后的处理函数
     */
    void Add(const std::string& method, const std::string& pattern, Handler handler);

    /**
     * @brief 按请求匹配路由并执行
     * @param req 请求对象
     * @param res 响应对象
     * @return 是否命中；失败返回 false（调用方回 404）
     */
    bool Route(CTL::Request& req, CTL::Response& res);

private:
    struct RouteEntry {
        std::string method;                   ///< HTTP 方法（GET / POST）
        std::vector<std::string> segments;    ///< 字面量段，{id} 处存参数名
        Handler handler;                      ///< 命中后的处理函数
    };

    std::vector<RouteEntry> routes_;          ///< 按注册顺序匹配，先注册先命中
};

} // namespace api::web
