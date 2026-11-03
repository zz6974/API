#pragma once
#include <memory>
#include <vector>
#include "api/common/Result.h"
#include "api/domain/PhotoMeta.h"

namespace api::app {

class ICameraChannel;
class ICameraChannelManager;
class IPhotoRepository;

// 摄像头用例服务：编排通道管理与照片存储
class CameraService {
public:
    CameraService(std::shared_ptr<ICameraChannelManager> channels,
                  std::shared_ptr<IPhotoRepository> photos,
                  int photoTimeoutMs = 5000);

    common::Result<std::vector<int>> GetOnlineDevices();
    common::Result<std::vector<char>> GetFrame(int deviceId);
    common::Result<domain::PhotoMeta> TakePhoto(int deviceId);
    common::Result<void> StartStream(int deviceId);
    common::Result<void> StopStream(int deviceId);

private:
    std::shared_ptr<ICameraChannel> FindOnline(int deviceId);

    std::shared_ptr<ICameraChannelManager> channels_;
    std::shared_ptr<IPhotoRepository> photos_;
    int photoTimeoutMs_;
};

} // namespace api::app
