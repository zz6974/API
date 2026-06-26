#ifndef CC_GZIP_H
#define CC_GZIP_H

#include "string"
#include "zlib.h"
#include "vector"

namespace CTL {
    class GZip {
    public:
    private:
    public:
        static int compressData(const char* src, int srcLen, char* dst, int dstLen);
        static int uncompressData(const char* src, int srcLen, char* dst, int dstLen);
        static std::vector<char> compress(const char* src, int srcLen);
        static std::vector<char> uncompress(const char* src, int srcLen);
    };
}



#endif
