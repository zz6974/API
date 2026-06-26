#ifndef CCFiles_H
#define CCFiles_H

#include "CC.h"

// 根据 C++ 版本选择合适的 filesystem 库
#if __cplusplus >= 201703L
    // C++17 及以上版本，使用标准 filesystem 库
    #include <filesystem>
    namespace CC_FS = std::filesystem;
#else
// C++11/14 版本，使用 ghc::filesystem
// 项目需要先包含 ghc::filesystem 库
#include <ghc/filesystem.hpp>
namespace CC_FS = ghc::filesystem;
#endif

#define KB_Unit_t 1024
#define MB_Unit_t (1024 * KB_Unit_t)
#define GB_Unit_t (1024 * MB_Unit_t)

// 根据不同的操作系统进行特定的设置
#ifdef _WIN32

#elif __linux__

#endif

/**
 * 结构体CCFileInfo用于存储文件信息。
 */
struct CCFileInfo
{
    CTL::String Name; // 文件名
    size_t Size{}; // 文件大小
    bool IsDirectory{}; // 是否为目录
    time_t LastWriteTime;
    CTL::String Path;
};

// 类型别名，用于简化目录迭代器的使用
typedef CC_FS::directory_iterator CCDir;
// 类型别名，用于简化目录条目的使用
typedef CC_FS::directory_entry CCDirEntry;

/**
 * CCFile类提供了对文件操作的支持，如删除、检查文件存在性等。
 */
namespace CTL{
    class File{
    public:
        // 构造函数，通过文件路径初始化CCFile对象
        File(const CTL::String& path);
        // 构造函数，通过目录条目初始化CCFile对象
        File(const CCDirEntry& entry);
        //----------------------------------------------------------------------------
        // 判断文件是否为目录
        bool isDirectory() const;
        // 删除文件或目录，如果isAll为true，则递归删除目录下的所有内容
        bool Delete(bool isAll = false) const;
        // 检查文件或目录是否存在
        bool isExists() const;
        // 判断文件是否为常规文件（非目录、符号链接等）
        bool isRegular() const;
        // 获取文件大小
        size_t GetFileSize() const;
        // 获取文件名
        CTL::String GetName() const;
        // 获取文件或目录的路径，如果relative为true，则返回相对路径
        CTL::String GetPath(bool relative = true) const;
        // 获取文件或目录的信息
        CCFileInfo GetFileInfo() const;
        //----------------------------------------------------------------------------
        // 获取指定路径的目录列表迭代器
        static CCDir DirectoryList(const CTL::String &path);
        // 标准化路径，确保路径格式在不同操作系统下的一致性
        static CTL::String NormalizePath(const CTL::String& Path);
        // 获取目录中的所有文件和子目录的信息
        static CCVector<CCFileInfo> GetDirectoryContents(const CTL::String& directoryPath);
        // 创建文件或目录，如果Dir为true，则尝试创建目录，否则创建文件
        static bool Create(const CTL::String& Path,bool Dir = false);
        /**
         *
         * @param src   源文件路径
         * @param dst   目标文件路径
         * @return 是否成功复制文件
         */
        static bool Copy(const CTL::String& src,const CTL::String& dst);
        /**
         * 获得相对路径
         * @param path 文件或目录路径
         * @param basePath 基础路径
         * @return 是否成功修改文件或目录名称
         */
        static CTL::String GetRelativePath(const CTL::String& path,const CTL::String& basePath);
        /**
         * 获取父目录路径
         * @param Path 文件或目录路径
         * @return 父目录路径
         */
        static CTL::String GetParentDir(const CTL::String& Path);
        /**
         *
         * @param OldPath   旧文件或目录路径
         * @param NewPath   新文件或目录路径
         * @return 是否成功修改文件或目录名称
         */
        static bool ReviseFileName(const CTL::String& OldPath,const CTL::String& NewPath);
        /**
         * 删除文件或目录，如果isAll为true，则递归删除目录下的所有内容
         * @param path 文件或目录路径
         * @param isAll 是否递归删除
         * @return 是否成功删除文件或目录
         */
        static bool Deletes(const CTL::String& path,bool isAll = false);
    private:
    // #ifdef _WIN32
    //     CTL::CCWString Path; // 文件或目录的路径
    // #elif __linux__
    //     CTL::String Path;
    // #endif
        CC_FS::path Path;
        // 将时间点转换为指定格式的字符串
        static time_t to_string(const CC_FS::file_time_type& time) {
    #if __cplusplus >= 202002L // C++20及以上版本
            // C++20提供了更直接的转换方法
            auto sysTime = std::chrono::clock_cast<std::chrono::system_clock>(time);
            return std::chrono::system_clock::to_time_t(sysTime);
    #elif __cplusplus >= 201703L // C++17版本
            // 对于C++17，需要手动转换
            auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                time - CC_FS::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            return std::chrono::system_clock::to_time_t(systemTime);
    #else // 更早的C++版本
            const auto cfTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                time - CC_FS::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            return std::chrono::system_clock::to_time_t(cfTime);
    #endif
        }
        // 删除文件,这是Delete函数的辅助函数
        bool deleteFile() const;
    };
}

typedef CTL::File CCFile;
#endif
