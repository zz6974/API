#pragma once
#include <cstddef>
#include <string>

namespace api::domain {

/**
 * @brief 照片元信息（值对象）
 *
 * 一次拍照成功的产物描述。
 */
struct PhotoMeta {
    std::string photoId;  ///< 唯一编号（即存储文件名）
    int deviceId = -1;    ///< 拍摄设备 ID
    std::size_t size = 0; ///< 照片字节数
};

struct MediaInfo {
        std::string codec;       ///< 编码名，如 "H264"
        int clockRate = 90000;   ///< 时钟频率
        int channels = 1;        ///< 声道数（音频）
        int payloadType = 96;    ///< RTP 负载类型（协商约定，非解析所得）
        std::string fmtp;        ///< 如 "profile-level-id=...;sprop-parameter-sets=..."
    };
} // namespace api::domain
