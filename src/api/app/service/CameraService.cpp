#include "api/app/service/CameraService.h"
#include "api/app/port/ICameraChannel.h"
#include "api/app/port/ICameraChannelManager.h"
#include "api/app/port/IPhotoRepository.h"
#include "api/domain/Protocol.h"

namespace api::app {

CameraService::CameraService(std::shared_ptr<ICameraChannelManager> channels,
                             std::shared_ptr<IPhotoRepository> photos,
                             int photoTimeoutMs)
    : channels_(std::move(channels)),
      photos_(std::move(photos)),
      photoTimeoutMs_(photoTimeoutMs) {
}

common::Result<std::vector<int>> CameraService::GetOnlineDevices() {
    return common::Result<std::vector<int>>::Ok(channels_->OnlineDevices());
}

common::Result<std::vector<char>> CameraService::GetFrame(int deviceId) {
    auto channel = FindOnline(deviceId);
    if (!channel) {
        return common::Result<std::vector<char>>::Fail(
            common::ErrorCode::kDeviceOffline, "设备不存在或已离线");
    }
    auto frame = channel->GetLatestFrame();
    if (frame.empty()) {
        // 设备在线但还没推过帧
        return common::Result<std::vector<char>>::Fail(
            common::ErrorCode::kNotFound, "该设备暂无可用帧");
    }
    return common::Result<std::vector<char>>::Ok(std::move(frame));
}

common::Result<domain::PhotoMeta> CameraService::TakePhoto(int deviceId) {
    auto channel = FindOnline(deviceId);
    if (!channel) {
        return common::Result<domain::PhotoMeta>::Fail(
            common::ErrorCode::kDeviceOffline, "设备不存在或已离线");
    }

    // 第一步：指令拍照并等待回传（通道内部条件变量同步，超时返回 kTimeout）
    auto photo = channel->TakePhoto(photoTimeoutMs_);
    if (photo.IsFail()) {
        return common::Result<domain::PhotoMeta>::Fail(photo.Code(), photo.Message());
    }
    if (photo.Value().empty()) {
        return common::Result<domain::PhotoMeta>::Fail(
            common::ErrorCode::kTimeout, "拍照超时，设备未回传照片");
    }

    // 第二步：落盘，拿到唯一 photoId
    auto saved = photos_->Save(deviceId, photo.Value());
    if (saved.IsFail()) {
        return common::Result<domain::PhotoMeta>::Fail(saved.Code(), saved.Message());
    }

    // 第三步：组装元信息返回
    domain::PhotoMeta meta;
    meta.photoId = saved.Value();
    meta.deviceId = deviceId;
    meta.size = photo.Value().size();
    return common::Result<domain::PhotoMeta>::Ok(meta);
}

common::Result<void> CameraService::StartStream(int deviceId) {
    auto channel = FindOnline(deviceId);
    if (!channel) {
        return common::Result<void>::Fail(
            common::ErrorCode::kDeviceOffline, "设备不存在或已离线");
    }
    channel->SendCommand(domain::protocol::kStreamStart);
    return common::Result<void>::Ok();
}

common::Result<void> CameraService::StopStream(int deviceId) {
    auto channel = FindOnline(deviceId);
    if (!channel) {
        return common::Result<void>::Fail(
            common::ErrorCode::kDeviceOffline, "设备不存在或已离线");
    }
    channel->SendCommand(domain::protocol::kStreamStop);
    return common::Result<void>::Ok();
}

std::shared_ptr<ICameraChannel> CameraService::FindOnline(int deviceId) {
    auto channel = channels_->Find(deviceId);
    // 会话存在但心跳超时也视为离线
    if (!channel || !channel->IsOnline()) {
        return nullptr;
    }
    return channel;
}

} // namespace api::app
