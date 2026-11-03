#pragma once
#include <memory>
#include <vector>
#include "api/app/port/ICameraChannel.h"

namespace api::app {

// 摄像头通道池（端口接口，由 infrastructure 层实现）
class ICameraChannelManager {
public:
    virtual ~ICameraChannelManager() = default;

    virtual void Start(int port) = 0;
    virtual void Stop() = 0;

    virtual std::vector<int> OnlineDevices() const = 0;
    virtual std::shared_ptr<ICameraChannel> Find(int deviceId) = 0;
};

} // namespace api::app
