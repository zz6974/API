#ifndef CC_PCM_H
#define CC_PCM_H

#include <cstdint>
#include <vector>
extern "C"{
#include <libswresample/swresample.h>
}


namespace CTL {
    enum FormatPCM {
        PCM_U8 = AV_SAMPLE_FMT_U8,
        PCM_S16 = AV_SAMPLE_FMT_S16,
        PCM_S32 = AV_SAMPLE_FMT_S32,
        PCM_FLT = AV_SAMPLE_FMT_FLT,
        PCM_NONE = AV_SAMPLE_FMT_NONE,
        PCM_U8P = AV_SAMPLE_FMT_U8P,
        PCM_S16P = AV_SAMPLE_FMT_S16P,
        PCM_S32P = AV_SAMPLE_FMT_S32P,
        PCM_FLT_P = AV_SAMPLE_FMT_FLTP,
        PCM_FMT_DBLP = AV_SAMPLE_FMT_DBLP,
        PCM_FMT_S64  = AV_SAMPLE_FMT_S64,
        PCM_FMT_S64P = AV_SAMPLE_FMT_S64P,
        PCM_FMT_NB = AV_SAMPLE_FMT_NB
    };
    class PCM_Format {
    public:
        int SamplingRate = 44100;
        int Format = FormatPCM::PCM_S32;
        int Channels = 2;
        [[nodiscard]] AVSampleFormat getFormat() const {
            AVSampleFormat out_sample_fmt;
            switch (Format) {
                case PCM_U8: out_sample_fmt = AV_SAMPLE_FMT_U8; break;
                case PCM_S16: out_sample_fmt = AV_SAMPLE_FMT_S16; break;
                case PCM_S32: out_sample_fmt = AV_SAMPLE_FMT_S32; break;
                case PCM_FLT: out_sample_fmt = AV_SAMPLE_FMT_FLT; break;
                case PCM_U8P: out_sample_fmt = AV_SAMPLE_FMT_U8P; break;
                case PCM_S16P: out_sample_fmt = AV_SAMPLE_FMT_S16P; break;
                case PCM_S32P: out_sample_fmt = AV_SAMPLE_FMT_S32P; break;
                case PCM_FLT_P: out_sample_fmt = AV_SAMPLE_FMT_FLTP; break;
                case PCM_FMT_DBLP: out_sample_fmt = AV_SAMPLE_FMT_DBLP; break;
                case PCM_FMT_S64: out_sample_fmt = AV_SAMPLE_FMT_S64; break;
                case PCM_FMT_S64P: out_sample_fmt = AV_SAMPLE_FMT_S64P; break;
                default: out_sample_fmt = AV_SAMPLE_FMT_S16; break;
            }
            return out_sample_fmt;
        }
        static FormatPCM getFormatPCM(const AVSampleFormat format) {
            FormatPCM out_format;
            switch (format) {
                case AV_SAMPLE_FMT_U8: out_format = PCM_U8; break;
                case AV_SAMPLE_FMT_S16: out_format = PCM_S16; break;
                case AV_SAMPLE_FMT_S32: out_format = PCM_S32; break;
                case AV_SAMPLE_FMT_FLT: out_format = PCM_FLT; break;
                case AV_SAMPLE_FMT_U8P: out_format = PCM_U8P; break;
                case AV_SAMPLE_FMT_S16P: out_format = PCM_S16P; break;
                case AV_SAMPLE_FMT_S32P: out_format = PCM_S32P; break;
                case AV_SAMPLE_FMT_FLTP: out_format = PCM_FLT_P; break;
                case AV_SAMPLE_FMT_DBLP: out_format = PCM_FMT_DBLP; break;
                case AV_SAMPLE_FMT_S64: out_format = PCM_FMT_S64; break;
                case AV_SAMPLE_FMT_S64P: out_format = PCM_FMT_S64P; break;
                default: out_format = PCM_NONE; break;
            }
            return out_format;
        }
    };
    struct MP4_Format {
        AVSampleFormat format = AV_SAMPLE_FMT_S16;
        int Channels = 2;
        int SamplingRate = 44100;
        AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;
    };
    class PCM {
    public:
        int length = -1;
        std::vector<uint8_t> data;
        int channel = 0;
        int sample_rate = 0;
        int format = 0;
    private:
    };
    class MP4PCM {
    public:
        int length = -1;
        std::vector<uint8_t> data;
        int pts = -1;
    };
}

#endif
