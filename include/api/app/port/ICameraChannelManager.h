#pragma once
#include <memory>
#include <vector>
#include "api/app/port/ICameraChannel.h"

namespace api::app {

/**
 * @brief 摄像头通道池
 *
 * 端口接口，由 infrastructure 层实现。
 */
class ICameraChannelManager {
public:
    virtual ~ICameraChannelManager() = default;

    /**
     * @brief 监听 TCP 端口，开始接受终端连接
     * @param port 监听端口
     */
    virtual void Start(int port) = 0;
    /** @brief 停止监听，回收全部会话与 ID */
    virtual void Stop() = 0;

    /**
     * @brief 当前在线设备
     * @return 在线设备 ID 列表
     */
    virtual std::vector<int> OnlineDevices() const = 0;
    /**
     * @brief 按 ID 查会话
     * @param deviceId 设备 ID
     * @return 会话指针，查不到返回 nullptr
     */
    virtual std::shared_ptr<ICameraChannel> Find(int deviceId) = 0;
};

} // namespace api::app
