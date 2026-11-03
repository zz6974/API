#pragma once
#include <string>
#include <vector>
#include "api/common/Result.h"

namespace api::app {

// 照片持久化（端口接口，由 infrastructure 层实现）
class IPhotoRepository {
public:
    virtual ~IPhotoRepository() = default;

    virtual common::Result<std::string> Save(int deviceId, const std::vector<char>& jpeg) = 0;
    virtual common::Result<std::vector<char>> Load(const std::string& photoId) = 0;
    virtual common::Result<std::vector<std::string>> List(int deviceId) = 0;
};

} // namespace api::app
