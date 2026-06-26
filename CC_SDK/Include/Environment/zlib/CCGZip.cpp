#include "CCGZip.h"

int CTL::GZip::compressData(const char *src, const int srcLen, char *dst, const int dstLen) {
    z_stream strm = {nullptr};
    // 初始化zlib流结构
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    // 设置输入输出缓冲区
    strm.avail_in = srcLen;
    strm.avail_out = dstLen;
    strm.next_in = (Bytef*)src;
    strm.next_out = (Bytef*)dst;
    // 初始化GZIP格式压缩
    // MAX_WBITS + 16 表示GZIP格式
    int ret = deflateInit2(&strm,
                          Z_BEST_COMPRESSION,  // 压缩级别
                          Z_DEFLATED,             // 压缩算法
                          MAX_WBITS + 16,         // GZIP格式
                          MAX_MEM_LEVEL,          // 内存级别
                          Z_DEFAULT_STRATEGY);    // 压缩策略
    if (ret != Z_OK) {
        return -1;
    }
    // 执行压缩
    ret = deflate(&strm, Z_SYNC_FLUSH);
    // 清理资源
    deflateEnd(&strm);
    if (ret != Z_OK) {
        return -1;
    }
    // 返回压缩后的实际大小
    return dstLen - strm.avail_out;
}

int CTL::GZip::uncompressData(const char *src, const int srcLen, char *dst, const int dstLen) {
    z_stream strm = {nullptr};
    // 初始化zlib流结构
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    // 设置输入输出缓冲区
    strm.avail_in = srcLen;
    strm.avail_out = dstLen;
    strm.next_in = (Bytef*)src;
    strm.next_out = (Bytef*)dst;
    // 初始化GZIP格式解压
    // MAX_WBITS + 16 表示GZIP格式
    int ret = inflateInit2(&strm, MAX_WBITS + 16);
    if (ret != Z_OK) {
        return -1;
    }
    // 执行解压
    ret = inflate(&strm, Z_SYNC_FLUSH);
    // 清理资源
    inflateEnd(&strm);
    if (ret != Z_OK && ret != Z_STREAM_END) {
        return -1;
    }
    // 返回解压后的实际大小
    return dstLen - strm.avail_out;
}

std::vector<char> CTL::GZip::compress(const char *src, const int srcLen) {
    std::vector<char> dst(srcLen * 2);  // 自动管理内存
    std::vector<char> result;
    const auto ret = compressData(src, srcLen, dst.data(), srcLen * 2);
    if (ret > 0) {
        result.assign(dst.begin(), dst.begin() + ret);
    }
    return result;  // 无需手动释放
}

std::vector<char> CTL::GZip::uncompress(const char *src, const int srcLen) {
    std::vector<char> dst(srcLen * 10);
    std::vector<char> result;
    const auto ret = uncompressData(src,srcLen,dst.data(),srcLen * 2);
    if (ret > 0) {
        result.assign(dst.begin(), dst.begin() + ret);
    }
    return result;
}
