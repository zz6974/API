#include "CCZip.h"
#include <cstring>
#include "CCFile.h"
#include "vector"
#include "fstream"

namespace fs_t = CC_FS;

bool CTL::Zip::UnzipFile(const std::string &zipPath, const std::string &outputDir) {
    // 打开 ZIP 文件
    const unzFile zipfile = unzOpen(zipPath.c_str());
    if (!zipfile) return false;

    // 获取全局信息
    unz_global_info global_info;
    if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK) {
        unzClose(zipfile);
        return false;
    }

    // 遍历所有文件
    char filename[256]{};

    for (uLong i = 0; i < global_info.number_entry; ++i) {
        unz_file_info file_info;
        if (unzGetCurrentFileInfo(zipfile, &file_info, filename, sizeof(filename),
                                 nullptr, 0, nullptr, 0) != UNZ_OK) {
            break;
                                 }

        // 构建输出路径
        std::string fullPath = outputDir + "/" + filename;

        // 检查是否为目录
        if (filename[strlen(filename) - 1] == '/') {
            // 创建目录（需要平台相关实现）
            fs_t::create_directories(fullPath);
        }
        else {
            // 打开当前文件
            if (unzOpenCurrentFile(zipfile) != UNZ_OK) break;

            // 创建输出文件
            FILE* out = fopen(fullPath.c_str(), "wb");
            if (!out) break;

            // 读取并写入文件内容
            int bytesRead;
            do {
                unsigned char buffer[FILE_BUFFER_SIZE];
                bytesRead = unzReadCurrentFile(zipfile, buffer, FILE_BUFFER_SIZE);
                if (bytesRead > 0) {
                    fwrite(buffer, 1, bytesRead, out);
                }
            } while (bytesRead > 0);

            fclose(out);
            unzCloseCurrentFile(zipfile);
        }

        // 移动到下一个文件
        if (i + 1 < global_info.number_entry) {
            if (unzGoToNextFile(zipfile) != UNZ_OK) break;
        }
    }

    unzClose(zipfile);
    return true;
}

bool CTL::Zip::ZipFolder(const std::string &folderPath, const std::string &zipPath) {
    zipFile zip = zipOpen64(zipPath.c_str(), APPEND_STATUS_CREATE);
    if (!zip) return false;

    const fs_t::path baseDir(folderPath);
    for (const auto& entry : fs_t::recursive_directory_iterator(baseDir, fs_t::directory_options::follow_directory_symlink)) {
        const fs_t::path& filePath = entry.path();
        const std::string relativePath = fs_t::relative(filePath, baseDir).string();

        if (fs_t::is_directory(filePath)) {
            // 添加目录条目
            zip_fileinfo zipInfo = {0};
            std::string dirEntry = relativePath + "/";
            zipOpenNewFileInZip(zip, dirEntry.c_str(), &zipInfo,
                               nullptr, 0, nullptr, 0, nullptr,
                               Z_DEFLATED, Z_DEFAULT_COMPRESSION);
            zipCloseFileInZip(zip);
        } else {
            // 添加文件
            std::ifstream file(filePath, std::ios::binary);
            if (!file.is_open()) continue;

            // 获取文件大小
            file.seekg(0, std::ios::end);
            const size_t size = file.tellg();
            file.seekg(0, std::ios::beg);

            // 读取文件内容
            std::vector<char> buffer(size);
            file.read(buffer.data(), size);
            file.close();

            // 准备 ZIP 文件信息
            zip_fileinfo zipInfo = {0};
            zipOpenNewFileInZip(zip, relativePath.c_str(), &zipInfo,
                               nullptr, 0, nullptr, 0, nullptr,
                               Z_DEFLATED, Z_DEFAULT_COMPRESSION);

            // 写入文件内容
            zipWriteInFileInZip(zip, buffer.data(), size);
            zipCloseFileInZip(zip);
        }
    }

    zipClose(zip, nullptr);
    return true;
}
