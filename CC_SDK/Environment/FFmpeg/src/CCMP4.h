#ifndef CC_MP4_H
#define CC_MP4_H

#include <cstdint>
#include <string>
#include <vector>
#include "CCPCM.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include "libavutil/opt.h"
}


namespace CTL {
    class MP4FrameInfo {
    public:
        //--------------------------------------------------------------------------------------------------------------
        size_t index;
        size_t size;
        int64_t pts;  // 显示时间戳
        int64_t dts;  // 解码时间戳
        bool is_key_frame;
        //--------------------------------------------------------------------------------------------------------------
    private:
        //--------------------------------------------------------------------------------------------------------------
    public:
        //--------------------------------------------------------------------------------------------------------------
    };

    class MP4Frame {
    public:
        int length = -1;
        std::vector<uint8_t> data;
        MP4PCM pcm;
    private:
    public:

    };

    struct MP4MediaInfo {
        int width;
        int height;
        int fps;
        int bitrate;
        std::string codec_name;
        int audio_sample_rate;
        int audio_channels;
    };

    struct MP4_Data {
        std::vector<std::vector<uint8_t>> video_data;
        std::vector<std::vector<uint8_t>> audio_data;
    };

    class MP4 {
    public:
        int frame_size = -1;
        int frame_count = -1;
        int audio_sample_rate = -1;
        int audio_channels = -1;
        int audio_bitrate = -1;
        int video_bitrate = -1;
        int video_fps = -1;
        int video_width = -1;
        int video_height = -1;
        std::string audio_codec_name;
        std::string video_codec_name;
        std::vector<MP4FrameInfo> frames;
        std::string filename;
    private:
        static void Println(const char* str);
    public:
        MP4();
        explicit MP4(const char* filename);
        ~MP4();
        void Init(const char* filename);
        [[nodiscard]] std::vector<MP4Frame> GetAllFramesVideo(MP4_Format* pcm_format = nullptr) const;
    private:
    };
}


#endif
