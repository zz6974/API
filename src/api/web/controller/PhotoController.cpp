#include "api/web/controller/PhotoController.h"
#include <utility>

namespace api::web {

PhotoController::PhotoController(std::shared_ptr<app::PhotoService> service)
    : service_(std::move(service)) {
}

void PhotoController::RegisterRoutes(Router& router) {
    // photoId 是字符串段，deviceId 用 GetIntParam 转整数
    router.Add("GET", "/api/photo/{photoId}",       [this](auto& req, auto& res, auto& p) { HandleGetPhoto(req, res, p); });
    router.Add("GET", "/api/photo/list/{deviceId}", [this](auto& req, auto& res, auto& p) { HandleListPhotos(req, res, p); });
}

void PhotoController::HandleGetPhoto(CTL::Request& req, CTL::Response& res, const PathParams& params) {
    auto it = params.find("photoId");
    if (it == params.end() || it->second.empty()) {
        // photoId 为字符串，不能用 GetIntParam，单独判空
        WriteError(res, 400, "缺少 photoId");
        return;
    }
    auto result = service_->GetPhoto(it->second);
    if (result.IsFail()) {
        WriteError(res, ToHttpStatus(result.Code()), result.Message());
        return;
    }
    WriteJpeg(res, result.Value()); // 照片本体以 image/jpeg 二进制回传
}

void PhotoController::HandleListPhotos(CTL::Request& req, CTL::Response& res, const PathParams& params) {
    int deviceId = GetIntParam(params, "deviceId");
    auto result = service_->ListPhotos(deviceId);
    if (result.IsFail()) {
        WriteError(res, ToHttpStatus(result.Code()), result.Message());
        return;
    }
    WriteJson(res, {{"photos", result.Value()}});
}

} // namespace api::web
