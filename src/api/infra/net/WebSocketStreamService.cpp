#include "api/infra/net/WebSocketStreamService.h"
#include "CCWebSocket.h"
#include "api/common/Logger.h"
#include <utility>

namespace api::infra {

WebSocketStreamService::WebSocketStreamService(
    std::shared_ptr<app::ICameraChannelManager> channels)
    : channels_(std::move(channels)) {
}

WebSocketStreamService::~WebSocketStreamService() {
    Stop();
}

void WebSocketStreamService::Start(const std::string& address, int port) {
    if (running_) return;

    websocket_ = new CTL::WebSocket();
    websocket_->Init(address.c_str(), port);
    websocket_->OnBind("/", CC_WSFunClass(StreamLoop),
                       nullptr, nullptr, nullptr, nullptr, nullptr);
    running_ = true;
    thread_ = std::thread([this]() { websocket_->Running(); });
    LOG_INFO("[WebSocketStream] 推流服务启动在 %s:%d", address.c_str(), port);
}

void WebSocketStreamService::Stop() {
    running_ = false;
    if (websocket_) {
        websocket_->Stop();
    }
    if (thread_.joinable()) {
        thread_.join();
    }
    delete websocket_;
    websocket_ = nullptr;
}

void WebSocketStreamService::StreamLoop(CTL::WebSocketInfo& info) {
    while (info.GetConnectFlag()) {
        auto devices = channels_->OnlineDevices();
        if (devices.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        auto channel = channels_->Find(devices[0]);
        if (!channel) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        auto frame = channel->GetLatestFrame();
        if (frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (!info.SendBinary(CTL::ByteArray(frame))) {
            LOG_WARN("[WebSocketStream] 发送帧失败");
        }
    }
}

} // namespace api::infra
