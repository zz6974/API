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

/**
 * @brief WebSocket 推流服务（由旧 URL::Init / URL::Onopen 迁移）
 *
 * 在独立端口向前端持续推送摄像头实时帧。
 */
class WebSocketStreamService {
public:
    /**
     * @brief 构造
     * @param channels 通道池（端口接口，用于取帧）
     */
    explicit WebSocketStreamService(std::shared_ptr<app::ICameraChannelManager> channels);
    ~WebSocketStreamService();

    /**
     * @brief 起 WS 服务线程，绑定根路径回调
     * @param address 监听地址
     * @param port 监听端口
     */
    void Start(const std::string& address, int port);
    /** @brief 停止推流服务 */
    void Stop();

private:
    /**
     * @brief 每个客户端连接一个循环
     * @param info 客户端连接信息
     */
    void StreamLoop(CTL::WebSocketInfo& info); // 取帧 -> 推二进制

    std::shared_ptr<app::ICameraChannelManager> channels_; ///< 取帧的通道池（端口接口）
    CTL::WebSocket* websocket_ = nullptr; ///< SDK WebSocket 对象
    std::thread thread_;                   ///< WS 运行线程
    bool running_ = false;                 ///< 运行开关
};

} // namespace api::infra
