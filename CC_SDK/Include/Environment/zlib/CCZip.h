#ifndef CC_ZIP_H
#define CC_ZIP_H

#include "string"
#include "unzip.h"
#include "zip.h"

#define FILE_BUFFER_SIZE 4096

namespace CTL {
    class Zip {
    public:
    private:
    public:
        /**
         * \brief 解压缩zip文件
         * \param zipPath zip文件的路径
         * \param outputDir 输出目录的路径
         * \return 如果解压成功，则为True
         */
        static bool UnzipFile(const std::string& zipPath, const std::string& outputDir);
        /**
         * \brief 压缩文件夹
         * \param folderPath 文件夹的路径
         * \param zipPath 输出zip文件的路径
         * \return 如果压缩成功，则为True
         */
        static bool ZipFolder(const std::string& folderPath, const std::string& zipPath);
    };
}

#endif
