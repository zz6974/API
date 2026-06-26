#ifndef CC_DATAFORMAT_H
#define CC_DATAFORMAT_H

#include "vector"

#define COMPRESSION_THRESHOLD 200

namespace CTL {
    class DataFormat {
    public:
    private:
    public:
        /**
         * \brief 压缩给定的数据
         *
         * \param input 要压缩的数据。
         * \return 压缩后的数据。
         */
        static std::vector<char> CompressData(const std::vector<char>& input);
        /**
         * \brief 解压缩给定的数据。
         *
         * \param compressed 要解压缩的数据。
         * \return 解压缩后的数据。
         */
        static std::vector<char> DecompressData(const std::vector<char>& compressed);
    };
}


#endif
