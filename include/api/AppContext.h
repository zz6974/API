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

// 组合根：唯一知道所有具体类的地方，负责装配与生命周期管理
class AppContext {
public:
    AppContext();
    ~AppContext();

    bool Init(const common::ServerConfig& config);
    void Run();
    void Shutdown();

private:
    common::ServerConfig config_;

    std::shared_ptr<infra::CameraChannelManager> channelManager_;
    std::unique_ptr<infra::UdpHeartbeatService> heartbeat_;
    std::unique_ptr<infra::WebSocketStreamService> webSocketStream_;

    std::shared_ptr<app::CameraService> cameraService_;
    std::shared_ptr<app::PhotoService> photoService_;
    std::shared_ptr<app::AuthService> authService_;
    std::shared_ptr<app::IIdAllocator> idAllocator_;

    web::Router router_;
    web::HttpServer httpServer_;
};

} // namespace api
