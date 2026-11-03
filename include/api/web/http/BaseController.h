#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "api/common/ErrorCode.h"
#include "api/common/Result.h"
#include "api/web/http/HttpTypes.h"

namespace CTL {
class Request;
class Response;
} // namespace CTL

namespace api::web {

/**
 * @brief 控制器基类
 *
 * 统一响应封装与参数工具。
 */
class BaseController {
protected:
    /**
     * @brief ErrorCode -> HTTP 状态码
     * @param code 业务错误码
     * @return HTTP 状态码（业务错误到协议层的唯一翻译点）
     */
    static int ToHttpStatus(common::ErrorCode code);

    /**
     * @brief 写 JSON 响应
     * @param res 响应对象
     * @param body JSON 内容
     * @param status HTTP 状态码（默认 200）
     */
    static void WriteJson(CTL::Response& res, const nlohmann::json& body, int status = 200);
    /**
     * @brief 写统一错误响应
     * @param res 响应对象
     * @param status HTTP 状态码
     * @param message 错误消息
     */
    static void WriteError(CTL::Response& res, int status, const std::string& message);
    /**
     * @brief 写 JPEG 二进制响应
     * @param res 响应对象
     * @param jpeg JPEG 字节
     */
    static void WriteJpeg(CTL::Response& res, const std::vector<char>& jpeg);

    /**
     * @brief 解析请求体 JSON
     * @param req 请求对象
     * @return 解析结果；空体或非法 JSON 返回 Fail
     */
    static common::Result<nlohmann::json> ParseJsonBody(CTL::Request& req);

    /**
     * @brief 从路径参数里取整数
     * @param params 路径参数表
     * @param name 参数名
     * @param fallback 缺失或非法时的兜底值（默认 -1）
     * @return 解析出的整数
     */
    static int GetIntParam(const PathParams& params, const std::string& name,
                           int fallback = -1) {
        auto it = params.find(name);
        if (it == params.end()) return fallback;
        try {
            return std::stoi(it->second);
        } catch (...) {
            return fallback;
        }
    }
};

} // namespace api::web
