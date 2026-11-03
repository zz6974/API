#pragma once
#include <memory>
#include "api/common/Config.h"
#include "api/web/http/HttpServer.h"
#include "api/web/http/Router.h"

namespace api::app {
class AuthService;
class CameraService;
class IIdAllocator;
class PhotoService;
} // namespace api::app

namespace api::infra {
class CameraChannelManager;
class UdpHeartbeatService;
class WebSocketStreamService;
} // namespace api::infra

namespace api {

/**
 * @brief 组合根
 *
 * 唯一知道所有具体类的地方，负责装配与生命周期管理。
 */
class AppContext {
public:
    AppContext();
    ~AppContext();

    /**
     * @brief 装配全部对象并启动后台服务
     * @param config 服务配置
     * @return 是否装配成功
     */
    bool Init(const common::ServerConfig& config);
    /**
     * @brief 阻塞运行 HTTP 服务
     *
     * 主线程调用。
     */
    void Run();
    /** @brief 逆序停止后台服务，可重复调用 */
    void Shutdown();

private:
    common::ServerConfig config_; ///< 运行配置副本

    // ---- infrastructure 具体实现（唯一知道它们的地方）----
    std::shared_ptr<infra::CameraChannelManager> channelManager_; ///< TCP 通道池（shared：被 service 引用）
    std::unique_ptr<infra::UdpHeartbeatService> heartbeat_;       ///< UDP 心跳服务
    std::unique_ptr<infra::WebSocketStreamService> webSocketStream_; ///< WS 推流服务

    // ---- application 用例服务 ----
    std::shared_ptr<app::CameraService> cameraService_; ///< 摄像头用例服务
    std::shared_ptr<app::PhotoService> photoService_;   ///< 照片用例服务
    std::shared_ptr<app::AuthService> authService_;     ///< 认证用例服务
    std::shared_ptr<app::IIdAllocator> idAllocator_;    ///< ID 分配器（端口接口，实际是 SlotIdAllocator）

    web::Router router_;              ///< 路由表（先于服务器构造，生命周期最长）
    web::HttpServer httpServer_;      ///< HTTP 服务器
};

} // namespace api
