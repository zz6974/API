#ifndef SRCCTL_CC_FF_MPEG_H
#define SRCCTL_CC_FF_MPEG_H

#include <string>

#include "CCPCM.h"

extern "C"{
#include <libavformat/avformat.h>
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
}

namespace CTL{
    struct SampleFormat{
        int sampling_rate = 44100;
        PCM_Format format = {};
        AVChannelLayout channel_layout = {};
        AVSampleFormat sample_fmt = AV_SAMPLE_FMT_S16;
    };
    // 音频重采样
    class Resample{
        AVSampleFormat out_sample_fmt;
        PCM_Format out_format;
        SwrContext* swr;
    public:
        Resample();
        ~Resample();
        bool init(const SampleFormat& format, const AVCodecContext* codec_ctx = nullptr);
        PCM convert(const AVFrame& frame);
    };
    class Mpeg{
        AVFormatContext *fmt_ctx;
    public:
        Mpeg();
        ~Mpeg();
        bool open(const std::string& path,const AVInputFormat* fmt = nullptr,
            AVDictionary** options = nullptr);
        void close();
        [[nodiscard]] AVFormatContext * get_fmt_ctx() const;
        int findStream(enum AVMediaType type,int wanted_stream_nb = -1,
            int related_stream = -1,const struct AVCodec** decoder_ret = nullptr,
            int flags = 0) const;
        int readFrame(AVPacket* pkt) const;
        [[nodiscard]] AVStream* getStreams(int index) const;
        [[nodiscard]] AVCodecID getCodecID(int index) const;
    };
    class AVDecoder{
        const AVCodec* codec;
        AVCodecContext* codec_ctx;
        AVCodecParameters* codec_par;
        bool is_open_t = false;
    public:
        AVDecoder();
        ~AVDecoder();
        bool init(enum AVCodecID id);
        bool init(const std::string& name);
        bool open(AVCodecParameters* par,AVDictionary** options = nullptr);
        [[nodiscard]] bool is_open() const;
        [[nodiscard]] AVCodecContext* get_codec_ctx() const;
        int send_packet(const AVPacket* pkt) const;
        int send_frame(const AVFrame *frame) const;
        int receive_frame(AVFrame* frame) const;
        int receive_packet(AVPacket* pkt) const;
    };
}


#endif