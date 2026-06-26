#include "CCFile.h"
#include <chrono>
#include <iomanip>
#include <fstream>
#include <chrono>

using namespace CTL;
File::File(const CTL::String &Path) {
#ifdef _WIN32
    CC_FS::path path = {};
    if (Path.is_str_utf8()) {
        path = Path.to_CCWString();
    }
    else {
        path = Path.c_str();
    }
    this->Path = path.c_str();
#elif __linux__
    this->Path = Path.c_str();
#endif
}

File::File(const CCDirEntry& dir) {
#ifdef _WIN32
    this->Path= CTL::String(dir.path().generic_string()).to_CCWString();
#elif __linux__
    this->Path = dir.path().generic_string();
#endif
}

bool File::isDirectory() const {
    return CC_FS::exists(Path.c_str()) && CC_FS::is_directory(Path.c_str());
}

bool File::Delete(const bool isAll) const {
    bool F = false;
    try {
        if (bool A = CC_FS::exists(Path.c_str())) {
            if(isAll) {
                CC_FS::remove_all(Path.c_str());
                F = true;
            }
            else {
                F = deleteFile();
            }
        }
        else {
            CC::ErrorPrintln("File not found");
        }
    }
    catch (const CC_FS::filesystem_error& e) {
        CC::ErrorPrintln(e.what());
    }
    return F;
}

bool File::isExists() const {
    return CC_FS::exists(Path.c_str());
}

bool File::isRegular() const {
    return CC_FS::is_regular_file(Path.c_str());
}

bool File::Create(const CTL::String& Path, const bool Dir) {
#ifdef _MSC_VER
    CC_FS::path path = {};
    if (Path.is_str_utf8()) {
        path = Path.to_CCWString();
    }
    else {
        path = Path.c_str();
    }
#else
    const CC_FS::path path = Path.c_str();
#endif
    if (Dir) {
        if (!CC_FS::exists(path.c_str())) {
            return CC_FS::create_directories(path.c_str());
        }
    }
    else {
        if (!CC_FS::exists(path.c_str())) {
            std::ofstream files(path);
            if (files) {
                // 写入内容或留空以创建空文件
                files << "";
                files.close();
                return true;
            }
        }
        return false;
    }
    return false;
}

bool File::Copy(const CTL::String &src, const CTL::String &dst) {
#ifdef _MSC_VER
    CC_FS::path src_t = {};
    CC_FS::path dst_t = {};
    if (src.is_str_utf8()) {
        src_t = src.to_CCWString();
    }
    else {
        src_t = src.c_str();
    }
    if (dst.is_str_utf8()) {
        dst_t = dst.to_CCWString();
    }
    else {
        dst_t = dst.c_str();
    }
#else
    const CC_FS::path src_t = src.c_str();
    const CC_FS::path dst_t = dst.c_str();
#endif
    if (CC_FS::exists(src_t.c_str()) && CC_FS::is_regular_file(src_t.c_str())) {
        if (!CC_FS::exists(dst_t.c_str())) {
            if (CC_FS::copy_file(src_t.c_str(), dst_t.c_str())) {
                return true;
            }
        }
    }
    return false;
}

CTL::String File::GetRelativePath(const CTL::String &path, const CTL::String &basePath) {
#ifdef _MSC_VER
    CC_FS::path srcPath = {};
    CC_FS::path base_aa = {};
    if (path.is_str_utf8()) {
        srcPath = path.to_CCWString();
    }
    else {
        srcPath = path.c_str();
    }
    if (basePath.is_str_utf8()) {
        base_aa = basePath.to_CCWString();
    }
    else {
        base_aa = basePath.c_str();
    }
    const String str = CC_FS::relative(srcPath, base_aa).generic_string();
    if (str.is_str_utf8()) {
        return str;
    }
    return str.to_utf8();
#else
    const CC_FS::path srcPath = path.c_str();
    const CC_FS::path base_aa = basePath.c_str();
    return CC_FS::relative(srcPath, base_aa).generic_string();
#endif

}

CTL::String File::GetParentDir(const CTL::String &Path){
#ifdef _MSC_VER
    CC_FS::path path = {};
    if (Path.is_str_utf8()) {
        path = Path.to_CCWString();
    }
    else {
        path = Path.c_str();
    }
    const String str = CC_FS::path(path.c_str()).parent_path().generic_string();
    if (str.is_str_utf8()) {
        return str;
    }
    return str.to_utf8();
#else
    const CC_FS::path path = Path.c_str();
    return CC_FS::path(path.c_str()).parent_path().generic_string();
#endif
}

bool File::ReviseFileName(const CTL::String &OldPath, const CTL::String &NewPath) {
    CC_FS::path old_path = {};
    CC_FS::path new_path = {};
#ifdef _MSC_VER
    if (OldPath.is_str_utf8()) {
        old_path = OldPath.to_CCWString();
    }
    else {
        old_path = OldPath.c_str();
    }
    if (NewPath.is_str_utf8()) {
        new_path = NewPath.to_CCWString();
    }
    else {
        new_path = NewPath.c_str();
    }
#else
    old_path = OldPath.c_str();
    new_path = NewPath.c_str();
#endif
    try {
        // 检查旧路径是否存在
        if (!CC_FS::exists(old_path.c_str())) {
            CC::ErrorPrintln("Source file not found");
            return false;
        }

        // 检查新路径是否已存在
        if (CC_FS::exists(new_path.c_str())) {
            CC::ErrorPrintln("Target file already exists");
            return false;
        }

        // 执行重命名操作
        CC_FS::rename(old_path.c_str(), new_path.c_str());
        return true;
    }
    catch (const CC_FS::filesystem_error& e) {
        CC::ErrorPrintln("Rename failed: " + CTL::String(e.what()));
        return false;
    }
}

bool File::Deletes(const CTL::String &path,const bool isAll) {
    const CTL::File file(path);
    if (file.isExists()) {
        return file.Delete(isAll);
    }
    return false;
}

size_t File::GetFileSize() const {
    return CC_FS::file_size(Path.c_str());
}

CTL::String File::GetName() const {
    return CC_FS::path(Path.c_str()).filename().generic_string();
}

CTL::String File::GetPath(bool relative) const {
    if(relative) {
        return CC_FS::path(Path.c_str()).relative_path().generic_string();
    }
    else {
        return CC_FS::path(Path.c_str()).parent_path().generic_string();
    }
}

CCFileInfo File::GetFileInfo() const {
    CCFileInfo info;
    info.IsDirectory = isDirectory();
    info.Name = GetName();
    info.Size = GetFileSize();
    return info;
}

CCDir File::DirectoryList(const CTL::String &P) {
#ifdef _MSC_VER
    CC_FS::path path = {};
    if (P.is_str_utf8()) {
        path = P.to_CCWString();
    }
    else {
        path = P.c_str();
    }
#else
    const CC_FS::path path = P.c_str();
#endif
    if (!CC_FS::exists(path.c_str())) {
        return {};
    }
    return CCDir(path.c_str(),CC_FS::directory_options::skip_permission_denied);
}

CTL::String File::NormalizePath(const CTL::String& Path) {
#ifdef _MSC_VER
    CC_FS::path path = {};
    if (Path.is_str_utf8()) {
        path = Path.to_CCWString();
    }
    else {
        path = Path.c_str();
    }
    const CC_FS::path normalizedPath = CC_FS::weakly_canonical(path.c_str());
    String str = normalizedPath.generic_string();
    if (str.is_str_utf8()) {
        return str;
    }
    return str.to_utf8();
#else
    const CC_FS::path path = Path.c_str();
    const CC_FS::path normalizedPath = CC_FS::weakly_canonical(path.c_str());
    return normalizedPath.generic_string();
#endif
}

CCVector<CCFileInfo> File::GetDirectoryContents(const CTL::String &directoryPath) {
#ifdef _MSC_VER
    CC_FS::path path = {};
    if (directoryPath.is_str_utf8()) {
        path = directoryPath.to_CCWString();
    }
    else {
        path = directoryPath.c_str();
    }
#else
    const CC_FS::path path = directoryPath.c_str();
#endif
    CCVector<CCFileInfo> fileInfos = {};
    try {
        if (CC_FS::exists(path) && CC_FS::is_directory(path)) {
            // 遍历目录中的所有条目
            for (const auto& entry : CC_FS::directory_iterator(path,CC_FS::directory_options::skip_permission_denied)) {
                CCFileInfo fileInfo;
                fileInfo.IsDirectory = CC_FS::is_directory(entry.status());
                fileInfo.Name = entry.path().filename().generic_string();
                if (!fileInfo.IsDirectory) {
                    fileInfo.Size = CC_FS::file_size(entry);
                }
                // 获取创建时间和最后修改时间
                const auto t = entry.last_write_time();
                fileInfo.LastWriteTime = to_string(t);
                fileInfo.Path = entry.path().generic_string();
                fileInfos.push_back(fileInfo);
            }
        }
        else {
            std::cerr << "The specified path does not exist or is not a directory." << std::endl;
        }
    }
    catch (const CC_FS::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "General error: " << e.what() << std::endl;
    }
    return fileInfos;
}

bool File::deleteFile() const {
    try {
        if (CC_FS::exists(Path.c_str())) {
            const bool result = CC_FS::remove(Path.c_str());
            return result;
        }
        else {
            return false;
        }
    }
    catch (const CC_FS::filesystem_error& e) {

        return false;
    }
}
