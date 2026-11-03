#pragma once
#include <memory>
#include <string>
#include <vector>
#include "api/common/Result.h"

namespace api::app {

class IPhotoRepository;

// 照片查询用例服务
class PhotoService {
public:
    explicit PhotoService(std::shared_ptr<IPhotoRepository> repository);

    common::Result<std::vector<char>> GetPhoto(const std::string& photoId);
    common::Result<std::vector<std::string>> ListPhotos(int deviceId);

private:
    std::shared_ptr<IPhotoRepository> repository_;
};

} // namespace api::app
