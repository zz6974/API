#pragma once
#include <string>
#include <vector>
#include "api/common/Result.h"

namespace api::app {

/**
 * @brief 一条与摄像头终端的 TCP 通道
 *
 * 端口接口，由 infrastructure 层实现。
 */
class ICameraChannel {
public:
    virtual ~ICameraChannel() = default;

    /**
     * @brief 设备 ID
     * @return 设备 ID，-1 表示还没注册
     */
    virtual int DeviceId() const = 0;
    /**
     * @brief 是否在线
     * @return 心跳未超时视为在线（默认 30 秒）
     */
    virtual bool IsOnline() const = 0;

    /**
     * @brief 向终端发指令（自动补换行）
     * @param command 指令内容
     */
    virtual void SendCommand(const std::string& command) = 0;
    /**
     * @brief 取最新一帧
     * @return 最新一帧 JPEG，无帧返回空
     */
    virtual std::vector<char> GetLatestFrame() = 0;
    /**
     * @brief 拍照并等待回传
     * @param timeoutMs 等待超时（毫秒）
     * @return 照片 JPEG；超时返回 kTimeout
     */
    virtual common::Result<std::vector<char>> TakePhoto(int timeoutMs) = 0;
};

} // namespace api::app
