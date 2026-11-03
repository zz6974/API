#pragma once
#include <shared_mutex>
#include <string>
#include <vector>
#include "api/app/port/IPhotoRepository.h"

namespace api::infra {

// 文件照片存储（由旧 PhotoManager 平移改造）：实现 app::IPhotoRepository 端口
class FilePhotoRepository : public app::IPhotoRepository {
public:
    explicit FilePhotoRepository(std::string photoDir = "./photos");
    ~FilePhotoRepository() override = default;

    common::Result<std::string> Save(int deviceId, const std::vector<char>& jpeg) override;
    common::Result<std::vector<char>> Load(const std::string& photoId) override;
    common::Result<std::vector<std::string>> List(int deviceId) override;

private:
    std::string photoDir_;
    mutable std::shared_mutex mutex_;
};

} // namespace api::infra
