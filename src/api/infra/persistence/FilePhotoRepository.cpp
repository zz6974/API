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

    // 随机后缀降低同毫秒写入的碰撞概率
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 0xFFFFF);
    char randStr[5];
    snprintf(randStr, sizeof(randStr), "%04x", dis(gen));

    // 与旧实现保持一致的文件名格式：毫秒时间戳 + 随机串
    std::string filename = "_" + std::to_string(ms) + "_" + randStr + ".jpg";
    std::string fullpath = photoDir_ + "/" + filename;

    {
        std::unique_lock<std::shared_mutex> lock(mutex_); // 独占锁：写盘期间禁止并发读目录
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
    std::ifstream in(fullPath, std::ios::binary | std::ios::ate); // ate：直接定位到末尾取文件大小
    if (!in) {
        return common::Result<std::vector<char>>::Fail(
            common::ErrorCode::kNotFound, "照片不存在: " + photoId);
    }

    size_t size = in.tellg(); // 文件大小
    in.seekg(0);              // 回到开头准备读

    std::vector<char> data(size);
    in.read(data.data(), static_cast<std::streamsize>(size));
    return common::Result<std::vector<char>>::Ok(std::move(data));
}

common::Result<std::vector<std::string>> FilePhotoRepository::List(int deviceId) {
    std::vector<std::string> result;
    std::string prefix = std::to_string(deviceId) + "_"; // 按文件名前缀过滤设备照片

#ifdef _WIN32
    // Win32 文件枚举 API，无需第三方库
    std::string pattern = photoDir_ + "/" + prefix + "*.jpg";
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            result.push_back(fd.cFileName);
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }
#else
    // POSIX 目录遍历
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
