#pragma once
#include <memory>
#include "api/app/service/CameraService.h"
#include "api/web/http/BaseController.h"
#include "api/web/http/Router.h"

namespace api::web {

/**
 * @brief 摄像头资源控制器
 *
 * 路由 /api/camera/*。HTTP 层只做参数解析与响应封装，业务全在 CameraService。
 */
class CameraController : public BaseController {
public:
    /**
     * @brief 构造
     * @param service 摄像头用例服务
     */
    explicit CameraController(std::shared_ptr<app::CameraService> service);

    /**
     * @brief 注册路由
     * @param router 路由表
     */
    void RegisterRoutes(Router& router); // 注册 5 条路由：列表 / 取帧 / 拍照 / 开流 / 停流

private:
    // 以下处理器职责一致：解析参数 -> 调用例服务 -> Result 转 HTTP 响应
    void HandleList(CTL::Request& req, CTL::Response& res, const PathParams& params);       ///< GET 设备列表
    void HandleFrame(CTL::Request& req, CTL::Response& res, const PathParams& params);      ///< GET 最新帧
    void HandleTakePhoto(CTL::Request& req, CTL::Response& res, const PathParams& params);  ///< POST 拍照
    void HandleStartStream(CTL::Request& req, CTL::Response& res, const PathParams& params); ///< POST 开始推流
    void HandleStopStream(CTL::Request& req, CTL::Response& res, const PathParams& params); ///< POST 停止推流

    std::shared_ptr<app::CameraService> service_; ///< 只依赖用例服务，不碰 infra 实现
};

} // namespace api::web
