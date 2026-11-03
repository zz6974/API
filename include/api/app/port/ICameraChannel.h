#pragma once
#include <string>
#include <vector>
#include "api/common/Result.h"

namespace api::app {

// 一条与摄像头终端的 TCP 通道（端口接口，由 infrastructure 层实现）
class ICameraChannel {
public:
    virtual ~ICameraChannel() = default;

    virtual int DeviceId() const = 0;
    virtual bool IsOnline() const = 0;

    virtual void SendCommand(const std::string& command) = 0;
    virtual std::vector<char> GetLatestFrame() = 0;
    virtual common::Result<std::vector<char>> TakePhoto(int timeoutMs) = 0;
};

} // namespace api::app
