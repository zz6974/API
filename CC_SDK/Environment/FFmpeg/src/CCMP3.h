#ifndef CC_MP3_H
#define CC_MP3_H

#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include "vector"
#include "CCFFmpeg.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include "libavutil/opt.h"
}

namespace CTL {
    class MP3Header {
    public:
        int size = -1;
        int frame_size = -1;
        int frame_length = -1;
        int samples_per_frame = -1;
        int channels = -1;
        int sampling_rate = -1;
        double frame_duration = -1;
        long long duration = -1;
        long long bit_rate = -1;
        bool is_vbr = false;
        bool has_cover = false;
    protected:
        AVFormatContext* format_ctx = nullptr;
        AVCodecContext* codec_ctx = nullptr;
        AVCodecParameters* codec_params = nullptr;
        int audio_stream_index = -1;
    public:
        MP3Header() = default;
        explicit MP3Header(const char* filename);
        ~MP3Header();
        void Init(const char* filename);
        static void SetLevelLog(int level);
        [[nodiscard]] AVFormatContext* GetFormatContext() const;
        [[nodiscard]] AVCodecParameters* GetCodecParameters() const;
        [[nodiscard]] AVCodecContext* GetAVCodecContext() const;
        [[nodiscard]] int GetAudioStreamIndex() const;
        std::vector<std::vector<uint8_t>> ExtractMP3Frames();
    private:
        [[nodiscard]] int GetFrameLength(int audio_stream_index) const;
    };

    class MP3Frame{
    public:
        int length = -1;
        std::vector<uint8_t> data;
    private:
    public:
        MP3Frame() = default;
        ~MP3Frame();
        static MP3Frame GetAudioFrameData(int frame_index,const std::vector<std::vector<uint8_t>>& frames);
        PCM DecodeAudioFrames(const MP3Header* header,const PCM_Format& format = {});
        std::vector<PCM> DecodeAudioFrames(const PCM_Format& format, const AVDecoder* decoder);
    };
}


#endif
