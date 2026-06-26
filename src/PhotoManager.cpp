#include "../include/PhotoManager.h"
#include "CCSystem.h"
#include <fstream>
#include <chrono>
#include <random>
#include <direct.h>
#include <sys/stat.h>
#include "../include/CameraSession.h"

PhotoManager & PhotoManager::Instance() {
    static PhotoManager instance;
    return instance;
}

//保存图片，返回图片ID
std::string PhotoManager::SavePhoto(int deviceId, CTL::ByteArray  &jpeg) {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    std::random_device rd;//随机数
    std::mt19937 gen(rd());//随机数生成器 , rd()种子
    std::uniform_int_distribution<> dis(0, 0xFFFFF);
    char randStr[5];
    snprintf(randStr, sizeof(randStr), "%04x", dis(gen));

    std::string filename =  "_"
                         + std::to_string(ms) + "_"
                         + randStr + ".jpg";
    std::string fullpath = photosDir_ +  "/" + filename;

    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        std::ofstream out(fullpath, std::ios::binary);
        if (!out) {
            std::cerr << ("[PhotoManager] 无法写入文件: {}", fullpath) << std::endl;
            return "";
        }
        out.write(jpeg.buffer(), jpeg.size());
    }
    std::cerr << ("[PhotoManager] 照片已保存: {}", filename) << std::endl;
    return filename;
}

//拿照片
std::vector<char> PhotoManager::GetPhoto(const std::string &photoId) {
    std::string fullPath = photosDir_ + "/" + photoId;

    std::shared_lock<std::shared_mutex> lock(mutex_);
    //std::ios::binary：以二进制模式打开
    //std::ios::ate：seek 到文件末尾（ate = at end），打开后，文件指针直接位于末尾，这样立刻可以获取文件大小。
    std::ifstream in(fullPath, std::ios::binary | std::ios::ate);
    if (!in) {
        return {};
    }

    size_t size = in.tellg();//tellg()：返回当前输入位置指针 ，结合之前的就是输出文件大小
    in.seekg(0);//seekg()：设置输入位置指针 ，回到文件开头

    std::vector<char> data(size);
    in.read(data.data(), size);

    return data;
}

//查看照片列表，包含摄像头ID
std::vector<std::string> PhotoManager::ListPhotos(int deviceId) const {
    std::vector<std::string> result;
    std::string prefix = std::to_string(deviceId) + "_";
    // 遍历 photos/ 目录，找以 "3_" 开头的文件
    // 简化实现：用系统命令 ls / dir
    // 生产环境应该用 opendir/readdir 或 std::filesystem
#ifdef _WIN32
    // Windows 用 FindFirstFile
    std::string pattern = photosDir_ + "/" + prefix + "*.jpg";
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            result.push_back(fd.cFileName);
        } while (FindNextFile(hFind, &fd));
        FindClose(hFind);
    }
#else
    // Linux 用 opendir/readdir
    DIR* dir = opendir(photosDir_.c_str());
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

    return result;
}

//创建文件夹
PhotoManager::PhotoManager() {
#ifdef _WIN32
    _mkdir(photosDir_.c_str());
#else
    _mkdir(photosDir_.c_str(),0755);
#endif
}
