#pragma once
#include <memory>
#include "api/app/service/CameraService.h"
#include "api/web/http/BaseController.h"
#include "api/web/http/Router.h"

namespace api::web {

// 摄像头资源控制器：路由 /api/camera/*
class CameraController : public BaseController {
public:
    explicit CameraController(std::shared_ptr<app::CameraService> service);

    void RegisterRoutes(Router& router);

private:
    void HandleList(CTL::Request& req, CTL::Response& res, const PathParams& params);
    void HandleFrame(CTL::Request& req, CTL::Response& res, const PathParams& params);
    void HandleTakePhoto(CTL::Request& req, CTL::Response& res, const PathParams& params);
    void HandleStartStream(CTL::Request& req, CTL::Response& res, const PathParams& params);
    void HandleStopStream(CTL::Request& req, CTL::Response& res, const PathParams& params);

    std::shared_ptr<app::CameraService> service_;
};

} // namespace api::web
