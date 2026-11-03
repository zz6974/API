#pragma once
#include <memory>
#include "api/app/service/PhotoService.h"
#include "api/web/http/BaseController.h"
#include "api/web/http/Router.h"

namespace api::web {

// 照片资源控制器：路由 /api/photo/*
class PhotoController : public BaseController {
public:
    explicit PhotoController(std::shared_ptr<app::PhotoService> service);

    void RegisterRoutes(Router& router);

private:
    void HandleGetPhoto(CTL::Request& req, CTL::Response& res, const PathParams& params);
    void HandleListPhotos(CTL::Request& req, CTL::Response& res, const PathParams& params);

    std::shared_ptr<app::PhotoService> service_;
};

} // namespace api::web
