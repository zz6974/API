#pragma once
#include <memory>
#include <string>
#include <vector>
#include "api/common/Result.h"

namespace api::app {

class IPhotoRepository;

/**
 * @brief 照片查询用例服务
 */
class PhotoService {
public:
    /**
     * @brief 构造
     * @param repository 照片存储（端口接口）
     */
    explicit PhotoService(std::shared_ptr<IPhotoRepository> repository);

    /**
     * @brief 按编号取照片
     * @param photoId 照片编号
     * @return 照片 JPEG
     */
    common::Result<std::vector<char>> GetPhoto(const std::string& photoId);
    /**
     * @brief 列某设备的照片编号
     * @param deviceId 设备 ID
     * @return 照片编号列表
     */
    common::Result<std::vector<std::string>> ListPhotos(int deviceId);

private:
    std::shared_ptr<IPhotoRepository> repository_; ///< 照片存储（端口接口）
};

} // namespace api::app
