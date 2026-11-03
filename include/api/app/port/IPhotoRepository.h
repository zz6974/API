#pragma once
#include <string>
#include <vector>
#include "api/common/Result.h"

namespace api::app {

/**
 * @brief 照片持久化
 *
 * 端口接口，由 infrastructure 层实现。
 */
class IPhotoRepository {
public:
    virtual ~IPhotoRepository() = default;

    /**
     * @brief 存照片
     * @param deviceId 拍摄设备 ID
     * @param jpeg 照片字节
     * @return 唯一 photoId
     */
    virtual common::Result<std::string> Save(int deviceId, const std::vector<char>& jpeg) = 0;
    /**
     * @brief 按编号取照片
     * @param photoId 照片编号
     * @return 照片字节
     */
    virtual common::Result<std::vector<char>> Load(const std::string& photoId) = 0;
    /**
     * @brief 列某设备的照片编号
     * @param deviceId 设备 ID
     * @return 照片编号列表
     */
    virtual common::Result<std::vector<std::string>> List(int deviceId) = 0;
};

} // namespace api::app
