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

// 控制器基类：统一响应封装与参数工具
class BaseController {
protected:
    static int ToHttpStatus(common::ErrorCode code);

    static void WriteJson(CTL::Response& res, const nlohmann::json& body, int status = 200);
    static void WriteError(CTL::Response& res, int status, const std::string& message);
    static void WriteJpeg(CTL::Response& res, const std::vector<char>& jpeg);

    static common::Result<nlohmann::json> ParseJsonBody(CTL::Request& req);

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
