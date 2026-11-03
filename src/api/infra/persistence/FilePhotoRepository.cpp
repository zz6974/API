#include "api/infra/persistence/FilePhotoRepository.h"
#include "api/common/Logger.h"
#include <chrono>
#include <direct.h>
#include <fstream>
#include <random>
#include <windows.h>

namespace api::infra {

FilePhotoRepository::FilePhotoRepository(std::string photoDir)
    : photoDir_(std::move(photoDir)) {
#ifdef _WIN32
    _mkdir(photoDir_.c_str());
#else
    mkdir(photoDir_.c_str(), 0755);
#endif
}

common::Result<std::string> FilePhotoRepository::Save(int deviceId,
                                                      const std::vector<char>& jpeg) {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 0xFFFFF);
    char randStr[5];
    snprintf(randStr, sizeof(randStr), "%04x", dis(gen));

    // 与旧实现保持一致的文件名格式
    std::string filename = "_" + std::to_string(ms) + "_" + randStr + ".jpg";
    std::string fullpath = photoDir_ + "/" + filename;

    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        std::ofstream out(fullpath, std::ios::binary);
        if (!out) {
            return common::Result<std::string>::Fail(
                common::ErrorCode::kIoError, "无法写入文件: " + fullpath);
        }
        out.write(jpeg.data(), static_cast<std::streamsize>(jpeg.size()));
    }
    LOG_INFO("[FilePhotoRepository] 照片已保存: %s", filename.c_str());
    return common::Result<std::string>::Ok(filename);
}

common::Result<std::vector<char>> FilePhotoRepository::Load(const std::string& photoId) {
    std::string fullPath = photoDir_ + "/" + photoId;

    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::ifstream in(fullPath, std::ios::binary | std::ios::ate);
    if (!in) {
        return common::Result<std::vector<char>>::Fail(
            common::ErrorCode::kNotFound, "照片不存在: " + photoId);
    }

    size_t size = in.tellg();
    in.seekg(0);

    std::vector<char> data(size);
    in.read(data.data(), static_cast<std::streamsize>(size));
    return common::Result<std::vector<char>>::Ok(std::move(data));
}

common::Result<std::vector<std::string>> FilePhotoRepository::List(int deviceId) {
    std::vector<std::string> result;
    std::string prefix = std::to_string(deviceId) + "_";

#ifdef _WIN32
    std::string pattern = photoDir_ + "/" + prefix + "*.jpg";
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            result.push_back(fd.cFileName);
        } while (FindNextFile(hFind, &fd));
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(photoDir_.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name.find(prefix) == 0) {
                result.push_back(name);
            }
        }
        closedir(dir);
    }
#endif

    return common::Result<std::vector<std::string>>::Ok(std::move(result));
}

} // namespace api::infra
