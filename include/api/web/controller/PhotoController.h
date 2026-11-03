#pragma once
#include <memory>
#include "api/app/service/PhotoService.h"
#include "api/web/http/BaseController.h"
#include "api/web/http/Router.h"

namespace api::web {

/**
 * @brief 照片资源控制器
 *
 * 路由 /api/photo/*。
 */
class PhotoController : public BaseController {
public:
    /**
     * @brief 构造
     * @param service 照片用例服务
     */
    explicit PhotoController(std::shared_ptr<app::PhotoService> service);

    /**
     * @brief 注册路由
     * @param router 路由表
     */
    void RegisterRoutes(Router& router); // 注册 2 条路由：取照片 / 列表

private:
    void HandleGetPhoto(CTL::Request& req, CTL::Response& res, const PathParams& params);  ///< 按编号回 JPEG
    void HandleListPhotos(CTL::Request& req, CTL::Response& res, const PathParams& params); ///< 列某设备照片编号

    std::shared_ptr<app::PhotoService> service_; ///< 照片用例服务
};

} // namespace api::web
