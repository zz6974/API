#pragma once
#include <memory>
#include <string>
#include <thread>
#include "api/app/port/ICameraChannelManager.h"

namespace CTL {
class WebSocket;
class WebSocketInfo;
} // namespace CTL

namespace api::infra {

// WebSocket 推流服务（由旧 URL::Init / URL::Onopen 迁移）：
// 在独立端口向前端持续推送摄像头实时帧
class WebSocketStreamService {
public:
    explicit WebSocketStreamService(std::shared_ptr<app::ICameraChannelManager> channels);
    ~WebSocketStreamService();

    void Start(const std::string& address, int port);
    void Stop();

private:
    void StreamLoop(CTL::WebSocketInfo& info);

    std::shared_ptr<app::ICameraChannelManager> channels_;
    CTL::WebSocket* websocket_ = nullptr;
    std::thread thread_;
    bool running_ = false;
};

} // namespace api::infra
