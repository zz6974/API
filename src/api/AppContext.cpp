#include "api/AppContext.h"
#include "api/app/port/IIdAllocator.h"
#include "api/app/service/AuthService.h"
#include "api/app/service/CameraService.h"
#include "api/app/service/PhotoService.h"
#include "api/common/Logger.h"
#include "api/infra/id/SlotIdAllocator.h"
#include "api/infra/net/CameraChannelManager.h"
#include "api/infra/net/UdpHeartbeatService.h"
#include "api/infra/net/WebSocketStreamService.h"
#include "api/infra/persistence/FilePhotoRepository.h"
#include "api/web/controller/AuthController.h"
#include "api/web/controller/CameraController.h"
#include "api/web/controller/IdController.h"
#include "api/web/controller/PhotoController.h"

namespace api {

AppContext::AppContext() = default;
AppContext::~AppContext() { Shutdown(); }

bool AppContext::Init(const common::ServerConfig& config) {
    config_ = config;

    // 1. 基础设施层：实例化端口实现
    idAllocator_ = std::make_shared<infra::SlotIdAllocator>(20);
    auto photoRepository = std::make_shared<infra::FilePhotoRepository>(config_.photoDir);
    channelManager_ = std::make_shared<infra::CameraChannelManager>(
        idAllocator_, config_.account, config_.password, config_.registerTimeoutMs);
    heartbeat_ = std::make_unique<infra::UdpHeartbeatService>();
    webSocketStream_ = std::make_unique<infra::WebSocketStreamService>(channelManager_);

    // 2. 应用层：依赖注入（只注入端口接口，不注入具体类）
    cameraService_ = std::make_shared<app::CameraService>(
        channelManager_, photoRepository, config_.photoTimeoutMs);
    photoService_ = std::make_shared<app::PhotoService>(photoRepository);
    authService_ = std::make_shared<app::AuthService>(config_.account, config_.password);

    // 3. 接口层：控制器 + 路由注册
    auto cameraController = std::make_shared<web::CameraController>(cameraService_);
    auto photoController = std::make_shared<web::PhotoController>(photoService_);
    auto authController = std::make_shared<web::AuthController>(authService_);
    auto idController = std::make_shared<web::IdController>(idAllocator_);

    cameraController->RegisterRoutes(router_);
    photoController->RegisterRoutes(router_);
    authController->RegisterRoutes(router_);
    idController->RegisterRoutes(router_);

    // 4. HTTP 服务器
    httpServer_.Init(config_, router_);

    // 5. 后台服务启动
    channelManager_->Start(config_.tcpPort);
    heartbeat_->Start(config_.httpAddress, config_.httpPort, 0);
    webSocketStream_->Start(config_.wsAddress, config_.wsPort);

    LOG_INFO("应用装配完成: HTTP :%d, TCP :%d, WS :%d",
             config_.httpPort, config_.tcpPort, config_.wsPort);
    return true;
}

void AppContext::Run() {
    httpServer_.Run();
}

void AppContext::Shutdown() {
    if (webSocketStream_) webSocketStream_->Stop();
    if (heartbeat_) heartbeat_->Stop();
    if (channelManager_) channelManager_->Stop();
}

} // namespace api
