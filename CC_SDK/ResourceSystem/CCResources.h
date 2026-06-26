#ifndef SRCCTL_CC_RESOURCES_H
#define SRCCTL_CC_RESOURCES_H

#include "unordered_map"
#include "string"
#include "mutex"
#include "vector"
#include "zlib.h"
#include "CCPak_Reader.h"

namespace CTL {
    struct Rcc {
        std::string path;
        std::vector<unsigned char> data;
    };
    enum Res_Type {
        RES_PAK = 0,
        RES_RCC = 1
    };
    class Resources {
        static std::mutex& get_lock() {
            static std::mutex lock;
            return lock;
        }
        static std::vector<unsigned char> decompress_python_zlib(const std::vector<unsigned char>& compressed_data) {
            if (compressed_data.empty()) {
                return {};
            }

            // 初始化解压流
            z_stream strm;
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;
            strm.avail_in = 0;
            strm.next_in = Z_NULL;

            // 使用 inflateInit2 处理 zlib 格式（Python 默认使用 zlib 格式）
            int ret = inflateInit2(&strm, 15 + 32);  // 15 是窗口大小，32 启用自动检测
            if (ret != Z_OK) {
                throw std::runtime_error(std::string("inflateInit2 失败: ") + zError(ret));
            }

            std::vector<unsigned char> uncompressed_data;
            const size_t chunk_size = 8192;  // 8KB 缓冲区
            unsigned char out[chunk_size];

            // 设置输入数据
            strm.avail_in = static_cast<uInt>(compressed_data.size());
            strm.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(compressed_data.data()));

            // 循环解压缩
            do {
                strm.avail_out = chunk_size;
                strm.next_out = out;

                ret = inflate(&strm, Z_NO_FLUSH);

                if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                    inflateEnd(&strm);
                    throw std::runtime_error(std::string("解压失败: ") + zError(ret));
                }

                // 计算实际输出的字节数
                size_t have = chunk_size - strm.avail_out;
                uncompressed_data.insert(uncompressed_data.end(), out, out + have);

            } while (strm.avail_out == 0);

            // 清理解压流
            inflateEnd(&strm);

            return uncompressed_data;
        }
    public:
        static std::unordered_map<std::string, std::vector<unsigned char>>* getResources() {
            static std::unordered_map<std::string, std::vector<unsigned char>> m_resources;
            return &m_resources;
        }
        static std::vector<unsigned char> get(const std::string& path,const Res_Type& type = RES_RCC) {
            if (type == RES_RCC) {
                std::unique_lock<std::mutex> lock(Resources::get_lock());
                const auto m_resources = Resources::getResources();
                if (m_resources->find(path) != m_resources->end()) {
                    return decompress_python_zlib(m_resources->at(path));
                }
            }
            else if (type == RES_PAK) {
                CTL::PakFile data_t = CTL::PakReader::read(path, true);
                if (data_t.Is_Success) {
                    const std::vector<unsigned char> data(data_t.FileData.begin(), data_t.FileData.end());
                    return decompress_python_zlib(data);
                }
                return {};
            }
            return {};
        }
        static void Init();
        static bool extractAllTo(const std::string& sourcePath, const std::string& outputDir) {
            return CTL::PakReader::extractAll(sourcePath, outputDir);
        }
    };
}

#endif
