#include "api/web/controller/CameraController.h"
#include <utility>

namespace api::web {

CameraController::CameraController(std::shared_ptr<app::CameraService> service)
    : service_(std::move(service)) {
}

void CameraController::RegisterRoutes(Router& router) {
    router.Add("GET",  "/api/camera/list",              [this](auto& req, auto& res, auto& p) { HandleList(req, res, p); });
    router.Add("GET",  "/api/camera/{id}/frame",        [this](auto& req, auto& res, auto& p) { HandleFrame(req, res, p); });
    router.Add("POST", "/api/camera/{id}/photo",        [this](auto& req, auto& res, auto& p) { HandleTakePhoto(req, res, p); });
    router.Add("POST", "/api/camera/{id}/stream/start", [this](auto& req, auto& res, auto& p) { HandleStartStream(req, res, p); });
    router.Add("POST", "/api/camera/{id}/stream/stop",  [this](auto& req, auto& res, auto& p) { HandleStopStream(req, res, p); });
}

void CameraController::HandleList(CTL::Request& req, CTL::Response& res, const PathParams&) {
    auto result = service_->GetOnlineDevices();
    if (result.IsFail()) {
        WriteError(res, ToHttpStatus(result.Code()), result.Message());
        return;
    }
    const auto& devices = result.Value();
    WriteJson(res, {{"devices", devices}, {"count", devices.size()}});
}

void CameraController::HandleFrame(CTL::Request& req, CTL::Response& res, const PathParams& params) {
    int deviceId = GetIntParam(params, "id");
    if (deviceId < 0) {
        WriteError(res, 400, "非法设备 ID");
        return;
    }
    auto result = service_->GetFrame(deviceId);
    if (result.IsFail()) {
        WriteError(res, ToHttpStatus(result.Code()), result.Message());
        return;
    }
    WriteJpeg(res, result.Value());
}

void CameraController::HandleTakePhoto(CTL::Request& req, CTL::Response& res, const PathParams& params) {
    int deviceId = GetIntParam(params, "id");
    if (deviceId < 0) {
        WriteError(res, 400, "非法设备 ID");
        return;
    }
    auto result = service_->TakePhoto(deviceId);
    if (result.IsFail()) {
        WriteError(res, ToHttpStatus(result.Code()), result.Message());
        return;
    }
    const auto& meta = result.Value();
    WriteJson(res, {
        {"photoId", meta.photoId},
        {"deviceId", meta.deviceId},
        {"size", meta.size}
    });
}

void CameraController::HandleStartStream(CTL::Request& req, CTL::Response& res, const PathParams& params) {
    int deviceId = GetIntParam(params, "id");
    auto result = service_->StartStream(deviceId);
    if (result.IsFail()) {
        WriteError(res, ToHttpStatus(result.Code()), result.Message());
        return;
    }
    WriteJson(res, {{"code", 200}});
}

void CameraController::HandleStopStream(CTL::Request& req, CTL::Response& res, const PathParams& params) {
    int deviceId = GetIntParam(params, "id");
    auto result = service_->StopStream(deviceId);
    if (result.IsFail()) {
        WriteError(res, ToHttpStatus(result.Code()), result.Message());
        return;
    }
    WriteJson(res, {{"code", 200}});
}

} // namespace api::web
