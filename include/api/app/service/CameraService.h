#pragma once
#include <memory>
#include <vector>
#include "api/common/Result.h"
#include "api/domain/PhotoMeta.h"

namespace api::app {

class ICameraChannel;
class ICameraChannelManager;
class IPhotoRepository;

/**
 * @brief 摄像头用例服务
 *
 * 编排通道管理与照片存储。
 */
class CameraService {
public:
    /**
     * @brief 构造（只注入端口接口，具体实现由 AppContext 装配）
     * @param channels 通道池
     * @param photos 照片存储
     * @param photoTimeoutMs 拍照等待回传超时（毫秒）
     */
    CameraService(std::shared_ptr<ICameraChannelManager> channels,
                  std::shared_ptr<IPhotoRepository> photos,
                  int photoTimeoutMs = 5000);

    /**
     * @brief 在线设备列表
     * @return 在线设备 ID 列表
     */
    common::Result<std::vector<int>> GetOnlineDevices();
    /**
     * @brief 取最新帧
     * @param deviceId 设备 ID
     * @return 最新帧 JPEG
     */
    common::Result<std::vector<char>> GetFrame(int deviceId);
    /**
     * @brief 拍照并落盘
     * @param deviceId 设备 ID
     * @return 照片元信息
     */
    common::Result<domain::PhotoMeta> TakePhoto(int deviceId);
    /**
     * @brief 指令终端开始推流
     * @param deviceId 设备 ID
     * @return 成功或失败
     */
    common::Result<void> StartStream(int deviceId);
    /**
     * @brief 指令终端停止推流
     * @param deviceId 设备 ID
     * @return 成功或失败
     */
    common::Result<void> StopStream(int deviceId);

private:
    /**
     * @brief 查设备且校验在线
     * @param deviceId 设备 ID
     * @return 会话指针，不在线返回 nullptr
     */
    std::shared_ptr<ICameraChannel> FindOnline(int deviceId);

    std::shared_ptr<ICameraChannelManager> channels_; ///< 通道池（端口接口）
    std::shared_ptr<IPhotoRepository> photos_;        ///< 照片存储（端口接口）
    int photoTimeoutMs_;                              ///< 拍照等待回传超时（毫秒）
};

} // namespace api::app
