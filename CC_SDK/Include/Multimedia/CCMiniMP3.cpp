#include "CCMiniMP3.h"
#include <cstdio>
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

namespace CTL {
    MiniMP3::MiniMP3(const char *filename) {
        Init(filename);
    }

    MiniMP3::~MiniMP3() {
        if (mp3d) {
            delete static_cast<mp3dec_t*>(mp3d);
            mp3d = nullptr;
        }
    }

    void MiniMP3::Init(const char *filename) {
        // 重置所有成员变量
        file_size = -1;
        frame_byte_size = -1;
        frame_count = -1;
        samples_per_frame = -1;
        channels = -1;
        sampling_rate = -1;
        frame_duration = -1;
        total_duration = -1;
        bit_rate = -1;
        is_vbr = false;
        has_cover = false;

        // 打开文件
        FILE* file = fopen(filename, "rb");
        if (!file) return;

        // 获取文件大小
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        // 读取整个文件
        mp3Data.resize(file_size);
        fread(mp3Data.data(), 1, file_size, file);
        fclose(file);

        // 初始化minimp3解码器
        if (!mp3d) mp3d = new mp3dec_t();
        mp3dec_init(static_cast<mp3dec_t*>(mp3d));

        // 解析第一帧获取基本信息
        mp3dec_frame_info_t info = {};
        const int samples = mp3dec_decode_frame(static_cast<mp3dec_t*>(mp3d),
                                         mp3Data.data(), file_size, nullptr, &info);

        if (samples > 0) {
            this->offsetof_t = info.frame_offset;
            frame_byte_size = info.frame_bytes - info.frame_offset;    // 每帧字节大小
            samples_per_frame = samples;           // 每帧样本数
            channels = info.channels;              // 声道数
            sampling_rate = info.hz;               // 采样率（Hz）
            frame_duration = static_cast<double>(samples_per_frame) / sampling_rate; // 帧时长（秒）
            bit_rate = info.bitrate_kbps;          // 比特率（bps）
            is_vbr = info.bitrate_kbps == 0;       // 是否为VBR
        }

        // 计算总帧数和总时长
        if (sampling_rate > 0 && samples_per_frame > 0) {
            // 遍历所有帧计算总帧数
            size_t offset = 0;
            frame_count = 0;
            int total_samples = 0;

            while (offset < mp3Data.size()) {
                mp3dec_frame_info_t frame_info = {};
                int frame_samples = mp3dec_decode_frame(
                    static_cast<mp3dec_t*>(mp3d),
                    mp3Data.data() + offset,
                    mp3Data.size() - offset,
                    nullptr,
                    &frame_info
                );

                if (frame_samples <= 0 || frame_info.frame_bytes <= 0) break;

                ++frame_count;
                total_samples += frame_samples;
                offset += frame_info.frame_bytes;
            }

            // 计算总时长（毫秒）
            total_duration = static_cast<long long>(
                (static_cast<double>(total_samples) / sampling_rate) * 1000
            ) / 1000;
        }
    }

    MiniMP3Frames MiniMP3::GetFrames(bool DecodingPCM) {
        MiniMP3Frames result;
        if (mp3Data.empty()) return result;

        const auto decoder = static_cast<mp3dec_t*>(mp3d);
        size_t offset = 0;
        while (offset < mp3Data.size()) {
            mp3dec_frame_info_t info = {};
            mp3d_sample_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME] = {0};
            // 查找下一帧
            int samples = mp3dec_decode_frame(decoder,
                                             mp3Data.data() + offset,
                                             mp3Data.size() - offset,
                                             pcm, &info);

            if (samples <= 0 || info.frame_bytes <= 0) break;

            // 将帧数据添加到结果中
            std::vector<uint8_t> frameData(
                mp3Data.begin() + offset,
                mp3Data.begin() + offset + info.frame_bytes
            );
            Frame frame;
            frame.frame = std::move(frameData);
            frame.length = frame.frame.size();
            result.frames.push_back(frame);
            if (DecodingPCM && (samples * info.channels == MINIMP3_MAX_SAMPLES_PER_FRAME)) {
                PCM_Int16 PCM;
                PCM.frame.assign(pcm, pcm + samples * info.channels);
                PCM.length = PCM.frame.size();
                result.PCM.push_back(PCM);
            }
            offset += info.frame_bytes;
        }

        result.length = result.frames.size();
        return result;
    }

    PCM_Int16 MiniMP3::GetPCM(const size_t frame_index) const {
        PCM_Int16 result;
        if (frame_index < mp3Data.size()) {
            const auto decoder = static_cast<mp3dec_t*>(mp3d);
            const size_t offset = this->offsetof_t;
            const size_t Index = offset + frame_index * frame_byte_size;
            mp3dec_frame_info_t info = {};
            mp3d_sample_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME] = {0};
            int samples = mp3dec_decode_frame(decoder,
                                             mp3Data.data() + Index,
                                             mp3Data.size() - Index,
                                             pcm, &info);
            if (samples <= 0 || info.frame_bytes <= 0) return {};
            result.frame.assign(pcm, pcm + samples * info.channels);
            result.length = result.frame.size();
            result.frames = (result.length / info.channels);
        }
        return std::move(result);
    }
}
