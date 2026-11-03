#pragma once
#include <shared_mutex>
#include <string>
#include <vector>
#include "api/app/port/IPhotoRepository.h"

namespace api::infra {

/**
 * @brief 文件照片存储（由旧 PhotoManager 平移改造）
 *
 * 实现 app::IPhotoRepository 端口。
 */
class FilePhotoRepository : public app::IPhotoRepository {
public:
    /**
     * @brief 构造（自动创建照片目录）
     * @param photoDir 照片目录
     */
    explicit FilePhotoRepository(std::string photoDir = "./photos");
    ~FilePhotoRepository() override = default;

    /**
     * @brief 存照片
     * @param deviceId 拍摄设备 ID
     * @param jpeg 照片字节
     * @return 文件名（即 photoId）
     */
    common::Result<std::string> Save(int deviceId, const std::vector<char>& jpeg) override;
    /**
     * @brief 读照片
     * @param photoId 照片编号
     * @return JPEG 字节
     */
    common::Result<std::vector<char>> Load(const std::string& photoId) override;
    /**
     * @brief 列某设备的照片
     * @param deviceId 设备 ID
     * @return 照片文件名列表
     */
    common::Result<std::vector<std::string>> List(int deviceId) override;

private:
    std::string photoDir_;                 ///< 照片目录（默认 ./photos，构造时自动创建）
    mutable std::shared_mutex mutex_;      ///< 写独占 / 读共享，保护目录内文件访问
};

} // namespace api::infra
