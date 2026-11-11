#include "api/app/service/PhotoService.h"
#include "api/app/port/IPhotoRepository.h"
#include <utility>

namespace api::app {

PhotoService::PhotoService(std::shared_ptr<IPhotoRepository> repository)
    : repository_(std::move(repository)) {
}

common::Result<std::vector<char>> PhotoService::GetPhoto(const std::string& photoId) {
    if (photoId.empty()) {
        // 参数校验前置：不合法请求不进入仓库层
        return common::Result<std::vector<char>>::Fail(
            common::ErrorCode::kInvalidArgument, "photoId 不能为空");
    }
    return repository_->Load(photoId);
}

common::Result<std::vector<std::string>> PhotoService::ListPhotos(int deviceId) {
    if (deviceId < 0) {
        return common::Result<std::vector<std::string>>::Fail(
            common::ErrorCode::kInvalidArgument, "非法设备 ID");
    }
    return repository_->List(deviceId);
}


} // namespace api::app
