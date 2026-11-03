#pragma once
#include <functional>
#include <map>
#include <string>

namespace CTL {
class Request;
class Response;
} // namespace CTL

namespace api::web {

/**
 * @brief 路径参数表
 *
 * pattern 中 {id} 等占位符的提取结果，键为占位名。
 */
using PathParams = std::map<std::string, std::string>;
/**
 * @brief 路由处理函数
 *
 * 直接操作 SDK 的 Request / Response，params 为占位符抓取值。
 */
using Handler = std::function<void(CTL::Request&, CTL::Response&, const PathParams&)>;

} // namespace api::web
