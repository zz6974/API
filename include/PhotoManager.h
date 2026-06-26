#ifndef API_PHOTOMANAGER_H
#define API_PHOTOMANAGER_H

#include <string>
#include <vector>
#include <shared_mutex>

#include "CCByteArray.h"

class PhotoManager {
public:
        static PhotoManager& Instance();

        // 存照片，返回 photoId（就是文件名）
        std::string SavePhoto(int deviceId, CTL::ByteArray & jpeg);
        // 按 photoId 取照片
        std::vector<char> GetPhoto(const std::string& photoId);
        // 列出某设备的所有照片
        std::vector<std::string> ListPhotos(int deviceId) const;

private:
        PhotoManager();                         // 构造时确保 photos/ 目录存在
        std::string photosDir_ = "./photos";
        mutable std::shared_mutex mutex_;       // 读写锁
};

#endif