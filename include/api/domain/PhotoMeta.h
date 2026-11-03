#pragma once
#include <cstddef>
#include <string>

namespace api::domain {

// 照片元信息（值对象）
struct PhotoMeta {
    std::string photoId;
    int deviceId = -1;
    std::size_t size = 0;
};

} // namespace api::domain
