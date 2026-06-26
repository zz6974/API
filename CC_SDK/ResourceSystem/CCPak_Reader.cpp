#include "CCPak_Reader.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <unordered_set>
#include <zlib.h>

// 根据 C++ 版本选择合适的 filesystem 库
#if __cplusplus >= 201703L
    #include <filesystem>
    namespace CC_FS = std::filesystem;
#else
    #include <ghc/filesystem.hpp>
    namespace CC_FS = ghc::filesystem;
#endif

std::mutex& CTL::PakReader::get_lock() {
    static std::mutex lock;
    return lock;
}

std::unordered_map<std::string, CTL::PakFile>* CTL::PakReader::getResources() {
    static std::unordered_map<std::string, PakFile> m_resources;
    return &m_resources;
}

// ======================== Pak 元数据全局缓存 ========================

struct CachedPakEntry {
    std::string fileName;
    size_t fileSize;  // 压缩后的大小
};

static std::mutex& getPakCacheLock() {
    static std::mutex lock;
    return lock;
}

static std::vector<CachedPakEntry>& getPakCache() {
    static std::vector<CachedPakEntry> cache;
    return cache;
}

static bool& isPakCacheLoaded() {
    static bool loaded = false;
    return loaded;
}

static void ensurePakCacheLoaded() {
    if (isPakCacheLoaded()) return;
    std::lock_guard<std::mutex> lock(getPakCacheLock());
    if (isPakCacheLoaded()) return;  // double-check

    auto& cache = getPakCache();
    cache.clear();

    std::ifstream file("./Resources/CCResources.pak", std::ios::binary | std::ios::in);
    if (!file.is_open()) {
        isPakCacheLoaded() = true;
        return;
    }

    while (file.good()) {
        CTL::PakInfo pakFile = {};
        file.read((char*)&pakFile, sizeof(CTL::PakInfo));

        if (!file.good() || file.gcount() != sizeof(CTL::PakInfo)) break;
        if (pakFile.FileNameSize == 0 || pakFile.FileNameSize > 1024) break;
        if (pakFile.totalSize < sizeof(CTL::PakInfo) + pakFile.FileNameSize) break;

        std::string fileName;
        fileName.resize(pakFile.FileNameSize);
        file.read((char*)fileName.data(), pakFile.FileNameSize);
        if (!file.good()) break;

        cache.push_back({fileName, pakFile.FileSize});

        size_t skipSize = pakFile.totalSize - sizeof(CTL::PakInfo) - pakFile.FileNameSize;
        file.seekg(skipSize, std::ios::cur);
    }
    file.close();
    isPakCacheLoaded() = true;
}

void CTL::PakReader::refreshPakCache() {
    std::lock_guard<std::mutex> lock(getPakCacheLock());
    getPakCache().clear();
    isPakCacheLoaded() = false;
}

// ======================== Pak 元数据全局缓存 END ========================

CTL::PakFile CTL::PakReader::read(const std::string& filePath, bool persistence) {
    {
        std::unique_lock<std::mutex> lock(get_lock());
        const auto resources = getResources();
        if (resources->find(filePath) != resources->end()) {
            return resources->at(filePath);
        }
    }
    PakFile F = {};
    F.Is_Success = false;
    std::ifstream file("./Resources/CCResources.pak", std::ios::binary | std::ios::in);
    if (file.is_open()) {
        while (file.good()) {
            PakInfo pakFile = {};
            file.read((char*)&pakFile, sizeof(PakInfo));
            // std::cout << "FileSize " << pakFile.FileSize << std::endl;
            // std::cout << "totalSize " << pakFile.totalSize << std::endl;
            // std::cout << "FileNameSize " << pakFile.FileNameSize << std::endl;
            if (!file.good() || file.gcount() != sizeof(PakInfo)) {
                break;
            }

            if (pakFile.FileNameSize == 0 || pakFile.FileNameSize > 1024) {
                break;
            }

            if (pakFile.totalSize < sizeof(PakInfo) + pakFile.FileNameSize) {
                break;
            }

            F.FileName.resize(pakFile.FileNameSize);
            file.read((char*)F.FileName.data(), pakFile.FileNameSize);

            if (!file.good()) {
                break;
            }

            if (F.FileName != filePath) {
                size_t skipSize = pakFile.totalSize - sizeof(PakInfo) - pakFile.FileNameSize;
                file.seekg(skipSize, std::ios::cur);
                continue;
            }

            if (pakFile.FileSize == 0 || pakFile.FileSize > 100 * 1024 * 1024) {
                break;
            }

            F.FileData.resize(pakFile.FileSize);
            file.read(F.FileData.data(), pakFile.FileSize);

            F.Is_Success = true;

            if (persistence) {
                std::unique_lock<std::mutex> lock(get_lock());
                const auto resources = getResources();
                resources->insert(std::pair<std::string, PakFile>(F.FileName,F));
            }
            break;
        }
        file.close();
    }
    return F;
}

void CTL::PakReader::uninstall(const std::string& filePath) {
    std::unique_lock<std::mutex> lock(get_lock());
    const auto resources = getResources();
    resources->erase(filePath);
}

bool CTL::PakReader::extractAll(const std::string& sourcePath, const std::string& outputDir) {
    // 创建输出根目录
    std::error_code ec;
    CC_FS::create_directories(outputDir, ec);
    if (ec) {
        return false;
    }

    std::ifstream file("./Resources/CCResources.pak", std::ios::binary | std::ios::in);
    if (!file.is_open()) {
        return false;
    }

    size_t extractedCount = 0;

    while (file.good()) {
        PakInfo pakFile = {};
        file.read((char*)&pakFile, sizeof(PakInfo));

        // 读取 PakInfo 失败，文件结束
        if (!file.good() || file.gcount() != sizeof(PakInfo)) {
            break;
        }

        // 文件名大小校验
        if (pakFile.FileNameSize == 0 || pakFile.FileNameSize > 1024) {
            break;
        }

        // totalSize 合法性校验
        if (pakFile.totalSize < sizeof(PakInfo) + pakFile.FileNameSize) {
            break;
        }

        // 读取文件名
        std::string fileName;
        fileName.resize(pakFile.FileNameSize);
        file.read((char*)fileName.data(), pakFile.FileNameSize);

        if (!file.good()) {
            break;
        }

        // 源路径过滤：精确匹配文件 或 前缀匹配目录
        if (!sourcePath.empty() &&
            fileName != sourcePath &&
            fileName.compare(0, sourcePath.size(), sourcePath) != 0) {
            // 跳过不匹配的文件数据
            file.seekg(pakFile.FileSize, std::ios::cur);
            continue;
        }

        // 文件大小校验
        if (pakFile.FileSize == 0 || pakFile.FileSize > 100 * 1024 * 1024) {
            break;
        }

        // 读取压缩的文件数据
        std::vector<unsigned char> compressedData(pakFile.FileSize);
        file.read((char*)compressedData.data(), pakFile.FileSize);

        if (!file.good()) {
            break;
        }

        // 构建输出路径
        CC_FS::path outPath = CC_FS::path(outputDir) / fileName;

        // 创建父目录
        CC_FS::create_directories(outPath.parent_path(), ec);
        if (ec) {
            continue;
        }

        // zlib 解压缩
        std::vector<unsigned char> uncompressedData;
        if (!compressedData.empty()) {
            z_stream strm = {};
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;

            int ret = inflateInit2(&strm, 15 + 32);  // 自动检测 zlib/gzip
            if (ret != Z_OK) {
                continue;
            }

            strm.avail_in = static_cast<uInt>(compressedData.size());
            strm.next_in = compressedData.data();

            const size_t chunkSize = 8192;
            unsigned char out[chunkSize];

            do {
                strm.avail_out = chunkSize;
                strm.next_out = out;

                ret = inflate(&strm, Z_NO_FLUSH);

                if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                    inflateEnd(&strm);
                    uncompressedData.clear();
                    break;
                }

                size_t have = chunkSize - strm.avail_out;
                uncompressedData.insert(uncompressedData.end(), out, out + have);
            } while (strm.avail_out == 0);

            inflateEnd(&strm);

            if (ret != Z_STREAM_END) {
                continue;
            }
        }

        // 写入文件
        std::ofstream outFile(outPath, std::ios::binary | std::ios::out);
        if (outFile.is_open()) {
            outFile.write((const char*)uncompressedData.data(), uncompressedData.size());
            outFile.close();
            ++extractedCount;
        }
    }

    file.close();
    return extractedCount > 0;
}

std::vector<CTL::Pak> CTL::PakReader::getPakList(const std::string &sourcePath) {
    ensurePakCacheLoaded();

    std::vector<Pak> pakList;
    std::unordered_set<std::string> seenNames;

    // 标准化 sourcePath：去除首尾 '/'
    std::string normalizedPath = sourcePath;
    while (!normalizedPath.empty() && normalizedPath.front() == '/') {
        normalizedPath.erase(0, 1);
    }
    while (!normalizedPath.empty() && normalizedPath.back() == '/') {
        normalizedPath.pop_back();
    }

    // 构建前缀：非根目录时加上 '/' 以确保精确层级匹配
    std::string prefix = normalizedPath.empty() ? "" : (normalizedPath + "/");

    for (const auto& entry : getPakCache()) {
        // 前缀过滤：非根目录时文件名必须以前缀开头
        if (!prefix.empty() && entry.fileName.compare(0, prefix.size(), prefix) != 0) {
            continue;
        }

        // 提取直接子条目名称
        std::string remainder = entry.fileName.substr(prefix.size());
        if (remainder.empty()) {
            continue;
        }

        size_t slashPos = remainder.find('/');
        std::string childName;
        bool isDir = false;

        if (slashPos == std::string::npos) {
            // 没有 '/' → 这是当前目录下的直接文件
            childName = remainder;
            isDir = false;
        } else {
            // 有 '/' → 这是当前目录下的子目录（只取第一级）
            childName = remainder.substr(0, slashPos);
            isDir = true;
        }

        if (childName.empty()) {
            continue;
        }
        if (seenNames.find(childName) != seenNames.end()) {
            continue;
        }
        seenNames.insert(childName);

        Pak item;
        item.name = childName;
        item.is_dir = isDir;
        item.size = isDir ? 0 : entry.fileSize;
        pakList.push_back(item);
    }

    return pakList;
}

std::vector<CTL::Pak> CTL::PakReader::getPakListAll(const std::string &sourcePath, bool filterDirectory) {
    ensurePakCacheLoaded();

    std::vector<Pak> pakList;
    std::unordered_set<std::string> seenDirs;

    // 标准化 sourcePath：去除首尾 '/'
    std::string normalizedPath = sourcePath;
    while (!normalizedPath.empty() && normalizedPath.front() == '/') {
        normalizedPath.erase(0, 1);
    }
    while (!normalizedPath.empty() && normalizedPath.back() == '/') {
        normalizedPath.pop_back();
    }

    // 构建前缀：非根目录时加上 '/' 以确保精确层级匹配
    std::string prefix = normalizedPath.empty() ? "" : (normalizedPath + "/");

    for (const auto& entry : getPakCache()) {
        // 前缀过滤
        if (!prefix.empty() && entry.fileName.compare(0, prefix.size(), prefix) != 0) {
            continue;
        }

        std::string remainder = entry.fileName.substr(prefix.size());
        if (remainder.empty()) {
            continue;
        }

        // 添加文件条目（完整相对路径）
        Pak fileEntry;
        fileEntry.is_dir = false;
        fileEntry.name = remainder;
        fileEntry.size = entry.fileSize;
        pakList.push_back(fileEntry);

        // 从文件路径推导所有父目录
        size_t pos = 0;
        while ((pos = remainder.find('/', pos)) != std::string::npos) {
            std::string dirName = remainder.substr(0, pos);
            if (seenDirs.find(dirName) == seenDirs.end()) {
                seenDirs.insert(dirName);
                Pak dirEntry;
                dirEntry.is_dir = true;
                dirEntry.name = dirName;
                dirEntry.size = 0;
                pakList.push_back(dirEntry);
            }
            ++pos;
        }
    }

    // 过滤目录：filterDirectory=true 时仅返回文件
    if (filterDirectory) {
        pakList.erase(
            std::remove_if(pakList.begin(), pakList.end(),
                [](const Pak& p) { return p.is_dir; }),
            pakList.end());
    }

    return pakList;
}
