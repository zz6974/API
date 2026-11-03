#include "api/web/http/BaseController.h"
#include "CCServlet/CCRequest.h"
#include "CCServlet/CCResponse.h"

namespace api::web {

// 错误码 -> HTTP 状态码映射：与 ErrorCode.h 一一对应
int BaseController::ToHttpStatus(common::ErrorCode code) {
    using EC = common::ErrorCode;
    switch (code) {
        case EC::kOk:              return 200;
        case EC::kInvalidArgument: return 400;
        case EC::kUnauthorized:    return 401;
        case EC::kNotFound:
        case EC::kDeviceOffline:   return 404;
        case EC::kNoIdAvailable:   return 503;
        case EC::kTimeout:         return 504;
        case EC::kIoError:
        case EC::kInternal:        return 500;
    }
    return 500;
}

void BaseController::WriteJson(CTL::Response& res, const nlohmann::json& body, int status) {
    std::string content = body.dump();
    res.SetHeader("Content-Type", "application/json; charset=utf-8");
    res.Write(content, status);
}

void BaseController::WriteError(CTL::Response& res, int status, const std::string& message) {
    WriteJson(res, {{"code", status}, {"msg", message}}, status);
}

void BaseController::WriteJpeg(CTL::Response& res, const std::vector<char>& jpeg) {
    res.SetHeader("Content-Type", "image/jpeg");
    res.Write(std::string(jpeg.begin(), jpeg.end()), 200);
}

common::Result<nlohmann::json> BaseController::ParseJsonBody(CTL::Request& req) {
    if (req.RequestData.Body.IsEmpty()) {
        return common::Result<nlohmann::json>::Fail(
            common::ErrorCode::kInvalidArgument, "Body 不能为空");
    }
    try {
        return common::Result<nlohmann::json>::Ok(
            nlohmann::json::parse(req.RequestData.Body.toString()));
    } catch (const std::exception&) {
        // 解析失败统一回参数错误，不区分具体异常类型
        return common::Result<nlohmann::json>::Fail(
            common::ErrorCode::kInvalidArgument, "Body 不是合法 JSON");
    }
}

} // namespace api::web
