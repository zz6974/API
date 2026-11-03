#pragma once
#include <cstddef>
#include <string>

namespace api::domain {

/**
 * @brief 照片元信息（值对象）
 *
 * 一次拍照成功的产物描述。
 */
struct PhotoMeta {
    std::string photoId;  ///< 唯一编号（即存储文件名）
    int deviceId = -1;    ///< 拍摄设备 ID
    std::size_t size = 0; ///< 照片字节数
};

} // namespace api::domain
