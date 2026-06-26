#ifndef SRCCTL_CC_PAK_READER_H
#define SRCCTL_CC_PAK_READER_H

#include <string>
#include <unordered_map>
#include <cstdint>
#include <iostream>
#include "mutex"
#include "vector"
#include "fstream"

namespace CTL {
    struct PakInfo {
        uint64_t FileNameSize;
        size_t FileSize;
        size_t totalSize; // FileNameSize + FileSize + sizeof(PakInfo)
    };
    struct PakFile {
        bool Is_Success;
        std::string FileName;
        std::vector<char> FileData;
    };
    struct Pak {
        bool is_dir;
        std::string name;
        size_t size;
    };
    class PakReader {
        static std::mutex& get_lock();
        static std::unordered_map<std::string, PakFile>* getResources();
    public:
        static PakFile read(const std::string& filePath,bool persistence);
        static void uninstall(const std::string& filePath);
        static bool extractAll(const std::string& sourcePath, const std::string& outputDir);
        static std::vector<Pak> getPakList(const std::string& sourcePath);
        static std::vector<Pak> getPakListAll(const std::string& sourcePath,bool filterDirectory);
        static void refreshPakCache();
    };
}

#endif