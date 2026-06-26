#include "CCMP3.h"
#include <codecvt>
#include <locale>

CTL::MP3Header::MP3Header(const char *filename) {
    this->Init(filename);
}

CTL::MP3Header::~MP3Header() {
    if (codec_ctx) {
        AVPacket empty_pkt = {nullptr};
        empty_pkt.data = nullptr;
        empty_pkt.size = 0;
        empty_pkt.stream_index = audio_stream_index;

        // 刷新解码器
        avcodec_send_packet(codec_ctx, &empty_pkt);
        av_packet_unref(&empty_pkt);

        // 处理剩余帧（可选）
        AVFrame* frame = av_frame_alloc();
        while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
            av_frame_unref(frame);
        }
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        codec_ctx = nullptr;
    }
    if (format_ctx) {
        avformat_close_input(&format_ctx);
        format_ctx = nullptr;
    }
}

void CTL::MP3Header::Init(const char *filename) {
    if (format_ctx || codec_ctx || codec_params) {
        avcodec_free_context(&codec_ctx);
        codec_ctx = nullptr;
        avformat_close_input(&format_ctx);
        format_ctx = nullptr;
        avcodec_parameters_free(&codec_params);
        codec_params = nullptr;
    }
    FILE* file = nullptr;
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    const std::wstring wide_path = converter.from_bytes(filename);
    file = _wfopen(wide_path.c_str(), L"rb");
#else
    file = fopen(filename, "rb");
#endif
    fseek(file, 0, SEEK_END);
    const long file_size = ftell(file);
    fclose(file);
    this->size = file_size;
    // 打开文件
    if (avformat_open_input(&format_ctx, filename, nullptr, nullptr) != 0) {
        std::cerr << "Cannot open the file: " << filename << std::endl;
        return;
    }
    // 获取文件流信息
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        std::cerr << "The stream information cannot be obtained" << std::endl;
        avformat_close_input(&format_ctx);
        return;
    }
    // 2. 获取歌曲时长（秒）
    if (format_ctx->duration != AV_NOPTS_VALUE) {
        this->duration = format_ctx->duration / (AV_TIME_BASE);
    }
    // 查找音频流
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }
    if (audio_stream_index == -1) {
        std::cerr << "No audio stream was found" << std::endl;
        avformat_close_input(&format_ctx);
        return;
    }
    this->frame_length = GetFrameLength(audio_stream_index);
    const AVStream* audio_stream = format_ctx->streams[audio_stream_index];
    codec_params = audio_stream->codecpar;
    if (codec_params) {
        // 计算帧时长(秒) = 每帧样本数 / 采样率
        if (codec_params->frame_size > 0 && codec_params->sample_rate > 0) {
            this->frame_duration = static_cast<double>(codec_params->frame_size)
                                / codec_params->sample_rate;
        }

        // MP3固定帧时长检查(适用于Layer III)
        if (this->frame_duration <= 0 && codec_params->codec_id == AV_CODEC_ID_MP3) {
            // MP3 Layer III 每帧固定包含1152个样本
            this->frame_duration = 1152.0 / codec_params->sample_rate;
        }
    }
    // 3. 帧大小（每帧样本数）
    this->samples_per_frame = codec_params->frame_size;
    // 4. 判断 VBR 还是 CBR
    if (codec_params->bit_rate < 0) {
        this->is_vbr = true;
    }
    this->bit_rate = codec_params->bit_rate / 1000;
    // 5. 检查是否有歌曲封面
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            this->has_cover = true;
            break;
        }
    }
    const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        std::cerr << "Unsupported codec!" << std::endl;
        avformat_close_input(&format_ctx);
        return;
    }
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "Failed to allocate codec context" << std::endl;
        avformat_close_input(&format_ctx);
        return;
    }
    if (avcodec_parameters_to_context(codec_ctx, codec_params) < 0) {
        std::cerr << "Failed to copy codec parameters" << std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return;
    }
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Failed to open codec" << std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return;
    }
    this->channels = codec_params->ch_layout.nb_channels;
    this->sampling_rate = codec_params->sample_rate;
    AVPacket* packet = av_packet_alloc();
    if (packet) {
        av_seek_frame(format_ctx, audio_stream_index, 0, AVSEEK_FLAG_BACKWARD);
        if (av_read_frame(format_ctx, packet) >= 0) {
            if (packet->stream_index == audio_stream_index) {
                // 关键修改: 将帧字节大小赋值给 frame_size
                this->frame_size = packet->size;
            }
        }
        av_packet_unref(packet);
        av_packet_free(&packet);
        av_seek_frame(format_ctx, audio_stream_index, 0, AVSEEK_FLAG_BACKWARD);
    }
}

void CTL::MP3Header::SetLevelLog(int level) {
    av_log_set_level(level);
}

AVFormatContext * CTL::MP3Header::GetFormatContext() const {
    return format_ctx;
}

AVCodecParameters * CTL::MP3Header::GetCodecParameters() const {
    return codec_params;
}

AVCodecContext * CTL::MP3Header::GetAVCodecContext() const {
    return codec_ctx;
}

int CTL::MP3Header::GetAudioStreamIndex() const {
    return audio_stream_index;
}

std::vector<std::vector<uint8_t>> CTL::MP3Header::ExtractMP3Frames() {
    std::vector<std::vector<uint8_t>> allFrames;
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "Failed to allocate packet" << std::endl;
        return allFrames;
    }

    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == audio_stream_index) {
            std::vector<uint8_t> frameData(packet->data, packet->data + packet->size);
            allFrames.push_back(std::move(frameData));
        }
        av_packet_unref(packet); // 仅在循环内释放
    }

    av_packet_free(&packet); // 循环结束后统一释放
    return allFrames;
}

int CTL::MP3Header::GetFrameLength(const int audio_stream_index) const {
    int total_frames = 0;

    // 改用av_packet_alloc创建packet
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "Failed to allocate packet" << std::endl;
        return -1;
    }

    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == audio_stream_index) {
            total_frames++;
        }
        av_packet_unref(packet);
    }

    av_seek_frame(format_ctx, audio_stream_index, 0, AVSEEK_FLAG_BACKWARD);
    av_packet_unref(packet);
    av_packet_free(&packet); // 新增：释放packet内存

    return total_frames;
}

CTL::MP3Frame::~MP3Frame() {
    this->length = -1;
    this->data.clear();
}

CTL::MP3Frame CTL::MP3Frame::GetAudioFrameData(const int frame_index,const std::vector<std::vector<uint8_t>>& frames) {
    MP3Frame frame_t;
    if (frames.size() > frame_index) {
        const auto frame = frames[frame_index];
        frame_t.length = frame.size();
        frame_t.data = frame;
    }
    return frame_t;
}

CTL::PCM CTL::MP3Frame::DecodeAudioFrames(const MP3Header* header,const PCM_Format& format) {
    CTL::PCM pcm_t;
    auto codec_ctx = header->GetAVCodecContext();
    if (!codec_ctx) return pcm_t;

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    if (!pkt || !frame) {
        if (pkt) av_packet_free(&pkt);
        if (frame) av_frame_free(&frame);
        return pcm_t;
    }

    pkt->data = reinterpret_cast<uint8_t*>(this->data.data());
    pkt->size = this->length;

    if (avcodec_send_packet(codec_ctx, pkt) >= 0) {
        while (avcodec_receive_frame(codec_ctx, frame) >= 0) {

            AVSampleFormat out_sample_fmt = format.getFormat();
            if (out_sample_fmt == AV_SAMPLE_FMT_NONE) {
                // 不支持的格式，使用默认的浮点
                out_sample_fmt = AV_SAMPLE_FMT_FLT;
            }

            // 修改后（新版API）
            AVChannelLayout in_ch_layout = codec_ctx->ch_layout;  // 直接使用现有布局
            AVChannelLayout out_ch_layout;
            av_channel_layout_default(&out_ch_layout, format.Channels);  // 创建新布局



            // 创建重采样器
            SwrContext* swr = nullptr;
            int ret = swr_alloc_set_opts2(&swr,
                                          &out_ch_layout, out_sample_fmt, format.SamplingRate,
                                          &in_ch_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate,
                                          0, nullptr);
            av_channel_layout_uninit(&out_ch_layout);  // 立即释放临时布局

            if (ret < 0) {
                std::cerr << "Failed to allocate SwrContext" << std::endl;
                if (swr) swr_free(&swr);
                continue;
            }
            if (swr_init(swr) < 0) {
                std::cerr << "Failed to initialize SwrContext" << std::endl;
                swr_free(&swr);
                continue;
            }

            // 计算最大输出样本数
            int max_out_samples = swr_get_out_samples(swr, frame->nb_samples);
            if (max_out_samples < 0) {
                swr_free(&swr);
                continue;
            }

            // 分配输出缓冲区
            uint8_t* out_data[AV_NUM_DATA_POINTERS] = { nullptr };
            int out_linesize = 0;
            if (av_samples_alloc(out_data, &out_linesize, format.Channels,
                                 max_out_samples, out_sample_fmt, 0) < 0) {
                av_freep(&out_data[0]);
                swr_free(&swr);
                continue;
            }

            // 执行重采样
            int out_samples = swr_convert(swr, out_data, max_out_samples,
                                          (const uint8_t**)frame->data, frame->nb_samples);
            if (out_samples < 0) {
                av_freep(&out_data[0]);
                swr_free(&swr);
                continue;
            }

            // 计算实际数据大小
            int data_size = av_samples_get_buffer_size(&out_linesize, format.Channels,
                                                      out_samples, out_sample_fmt, 1);
            if (data_size > 0) {
                pcm_t.data.assign(out_data[0], out_data[0] + data_size);
                pcm_t.length = data_size;
            }

            // 释放重采样器和输出缓冲区
            av_freep(&out_data[0]);
            swr_close(swr);
            if (swr) {
                swr_free(&swr); // 显式释放 SwrContext
                swr = nullptr;
            }
        }
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);
    return pcm_t;
}

std::vector<CTL::PCM> CTL::MP3Frame::DecodeAudioFrames(const PCM_Format& format, const AVDecoder* decoder){
    std::vector<PCM> pcm_t;
    if (!decoder || !decoder->is_open()) {
        std::cout << "Decoder is not open" << std::endl;
        return pcm_t;
    }
    AVPacket pkt = {};
    pkt.data = this->data.data();
    pkt.size = this->length;
    pkt.stream_index = 0;
    if (decoder->send_packet(&pkt) >= 0) {
        Resample resample;
        AVFrame frame = {};
        while (decoder->receive_frame(&frame) >= 0) {
            SampleFormat format_t;
            format_t.format = format;
            if (resample.init(format_t,decoder->get_codec_ctx())) {
                const auto buffer = resample.convert(frame);
                pcm_t.push_back(buffer);
            }
            av_frame_unref(&frame);
        }
        av_packet_unref(&pkt);
    }
    return pcm_t;
}



