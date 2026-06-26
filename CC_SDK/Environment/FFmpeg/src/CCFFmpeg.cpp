#include "CCFFmpeg.h"

#include <iostream>


CTL::Resample::Resample(){
    out_format = {};
    out_sample_fmt = AV_SAMPLE_FMT_S16;
    swr = nullptr;
}

CTL::Resample::~Resample(){
    swr_close(swr);
    if (swr) {
        swr_free(&swr);
        swr = nullptr;
    }
}

bool CTL::Resample::init(const SampleFormat& format, const AVCodecContext* codec_ctx){
    out_format = format.format;
    out_sample_fmt = format.format.getFormat();
    AVChannelLayout in_ch_layout = format.channel_layout;  // 直接使用现有布局
    AVChannelLayout out_ch_layout;
    AVSampleFormat in_sample_fmt = format.sample_fmt;
    int in_sample_rate = format.sampling_rate;
    av_channel_layout_default(&out_ch_layout, format.format.Channels);  // 创建新布局
    if (codec_ctx) {
        in_ch_layout = codec_ctx->ch_layout;
        in_sample_fmt = codec_ctx->sample_fmt;
        in_sample_rate = codec_ctx->sample_rate;
    }
    const int ret = swr_alloc_set_opts2(&swr,&out_ch_layout, out_sample_fmt, format.format.SamplingRate,
      &in_ch_layout, in_sample_fmt, in_sample_rate,0, nullptr);
    av_channel_layout_uninit(&out_ch_layout);
    if (ret < 0) {
        std::cout << "CTL::Resample::init swr_alloc_set_opts2 error" << std::endl;
        if (swr) {
            swr_free(&swr);
            swr = nullptr;
        }
        return false;
    }
    if (swr_init(swr) < 0) {
        std::cout << "CTL::Resample::init Failed to initialize SwrContext" << std::endl;
        swr_free(&swr);
        return false;
    }
    return true;
}

CTL::PCM CTL::Resample::convert(const AVFrame& frame){
    PCM out_stream;
    int max_out_samples = swr_get_out_samples(swr, frame.nb_samples);
    if (max_out_samples < 0) {
        swr_free(&swr);
        return out_stream;
    }
    uint8_t* out_data[AV_NUM_DATA_POINTERS] = { nullptr };
    int out_linesize = 0;
    if (av_samples_alloc(out_data, &out_linesize, out_format.Channels,max_out_samples, out_sample_fmt, 0) < 0) {
        av_freep(&out_data[0]);
        swr_free(&swr);
        return out_stream;
    }
    const uint8_t *in_data[AV_NUM_DATA_POINTERS] = { nullptr };
    for (int i = 0; i < AV_NUM_DATA_POINTERS; i++) {
        in_data[i] = frame.data[i];
    }
    int out_samples = swr_convert(swr, out_data, max_out_samples,in_data, frame.nb_samples);
    if (out_samples < 0) {
        av_freep(&out_data[0]);
        swr_free(&swr);
        return out_stream;
    }
    int data_size = av_samples_get_buffer_size(&out_linesize, out_format.Channels,
                                      out_samples, out_sample_fmt,0);
    if (data_size > 0) {
        out_stream.data.assign(out_data[0], out_data[0] + data_size);
        out_stream.length = data_size;
        out_stream.channel = out_format.Channels;
        out_stream.format = out_sample_fmt;
        out_stream.sample_rate = out_format.SamplingRate;
    }
    av_freep(&out_data[0]);
    return out_stream;
}

CTL::Mpeg::Mpeg(){
    fmt_ctx = nullptr;
}

CTL::Mpeg::~Mpeg(){
    if (fmt_ctx) {
        avformat_free_context(fmt_ctx);
        fmt_ctx = nullptr;
    }
}

bool CTL::Mpeg::open(const std::string& path, const AVInputFormat* fmt, AVDictionary** options){
    int ret = avformat_open_input(&fmt_ctx, path.c_str(), fmt, options);
    if (ret < 0) {
        return false;
    }
    return true;
}

void CTL::Mpeg::close(){
    avformat_close_input(&fmt_ctx);
}

AVFormatContext* CTL::Mpeg::get_fmt_ctx() const{
    return fmt_ctx;
}

int CTL::Mpeg::findStream(const enum AVMediaType type, const int wanted_stream_nb, const int related_stream,
    const struct AVCodec** decoder_ret, const int flags) const{
    const int index = av_find_best_stream(fmt_ctx, type, wanted_stream_nb, related_stream, decoder_ret, flags);
    if (index < 0) {
        return -1;
    }
    return index;
}

int CTL::Mpeg::readFrame(AVPacket* pkt) const{
    return av_read_frame(fmt_ctx, pkt);
}

AVStream* CTL::Mpeg::getStreams(const int index) const{
    if (fmt_ctx->nb_streams <= index) {
        return nullptr;
    }
    const auto stream = fmt_ctx->streams[index];
    if (!stream) {
        return nullptr;
    }
    return stream;
}

AVCodecID CTL::Mpeg::getCodecID(const int index) const{
    const auto stream = getStreams(index);
    if (!stream) {
        return {};
    }
    if (stream->codecpar) {
        return stream->codecpar->codec_id;
    }
    return {};
}

CTL::AVDecoder::AVDecoder(){
    codec = nullptr;
    codec_ctx = nullptr;
    codec_par = nullptr;
}

CTL::AVDecoder::~AVDecoder(){
    if (codec) {
        codec = nullptr;
    }
    if (codec_ctx) {
        avcodec_free_context(&codec_ctx);
        codec_ctx = nullptr;
    }
    is_open_t = false;
}

bool CTL::AVDecoder::init(const enum AVCodecID id){
    codec = avcodec_find_decoder(id);
    if (!codec) {
        return false;
    }
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        return false;
    }
    return true;
}

bool CTL::AVDecoder::init(const std::string& name){
    codec = avcodec_find_decoder_by_name(name.c_str());
    if (!codec) {
        return false;
    }
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        return false;
    }
    return true;
}

bool CTL::AVDecoder::open(AVCodecParameters* par, AVDictionary** options){
    this->codec_par = par;
    if (par) {
        if (avcodec_parameters_to_context(codec_ctx, par) < 0) {
            return false;
        }
    }
    if (avcodec_open2(codec_ctx, codec, options) < 0) {
        return false;
    }
    is_open_t = true;
    return true;
}

bool CTL::AVDecoder::is_open() const{
    return is_open_t;
}

AVCodecContext* CTL::AVDecoder::get_codec_ctx() const{
    return codec_ctx;
}

int CTL::AVDecoder::send_packet(const AVPacket* pkt) const{
    return avcodec_send_packet(codec_ctx,pkt);
}

int CTL::AVDecoder::send_frame(const AVFrame* frame) const{
    return avcodec_send_frame(codec_ctx,frame);
}

int CTL::AVDecoder::receive_frame(AVFrame* frame) const{
    return avcodec_receive_frame(codec_ctx, frame);
}

int CTL::AVDecoder::receive_packet(AVPacket* pkt) const{
    return avcodec_receive_packet(codec_ctx, pkt);
}
