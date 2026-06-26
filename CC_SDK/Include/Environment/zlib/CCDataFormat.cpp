#include "CCDataFormat.h"

#include <cstring>
#include <stdexcept>
#include "zip.h"

std::vector<char> CTL::DataFormat::CompressData(const std::vector<char> &input) {
    if (input.size() < COMPRESSION_THRESHOLD) {
        return input;
    }
    // 计算压缩后最大长度
    uLong maxCompressedSize = compressBound(input.size());
    std::vector<char> compressed(maxCompressedSize + sizeof(uLong)); // +4字节存储原始长度

    // 在头部存储原始数据长度（4字节）
    auto originalSize = static_cast<uLong>(input.size());
    memcpy(compressed.data(), &originalSize, sizeof(uLong));

    // 执行压缩
    int result = compress2(
        reinterpret_cast<Bytef*>(compressed.data()) + sizeof(uLong), // 偏移4字节
        &maxCompressedSize,
        reinterpret_cast<const Bytef*>(input.data()),
        input.size(),
        Z_BEST_COMPRESSION // 最高压缩率
    );

    if (result != Z_OK) {
        throw std::runtime_error("Compression failed");
    }

    // 调整vector大小为实际压缩后长度 + 4字节头
    compressed.resize(maxCompressedSize + sizeof(uLong));
    return compressed;
}

std::vector<char> CTL::DataFormat::DecompressData(const std::vector<char> &compressed) {
    if (compressed.size() < sizeof(uLong)) {
        throw std::runtime_error("Invalid compressed data");
    }

    // 从头部提取原始数据长度
    uLong originalSize;
    memcpy(&originalSize, compressed.data(), sizeof(uLong));

    std::vector<char> output(originalSize);

    uLong decompressedSize = originalSize;
    int result = uncompress(
        reinterpret_cast<Bytef*>(output.data()),
        &decompressedSize,
        reinterpret_cast<const Bytef*>(compressed.data()) + sizeof(uLong), // 跳过4字节头
        compressed.size() - sizeof(uLong)
    );

    if (result != Z_OK || decompressedSize != originalSize) {
        throw std::runtime_error("Decompression failed");
    }

    return output;
}
