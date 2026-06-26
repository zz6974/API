#include "CCMP4.h"
#include "iostream"
#include <queue>
#include <stdexcept>



namespace CTL {
    static int64_t ConvertPts(AVStream* stream, int64_t pts, AVRational target_time_base) {
        return av_rescale_q(pts, stream->time_base, target_time_base);
    }

    void MP4::Println(const char *str) {
        std::cout << str << std::endl;
    }

    MP4::MP4() {

    }

    MP4::MP4(const char *filename) {
        Init(filename);
    }

    MP4::~MP4() {

    }

    void MP4::Init(const char *filename) {
    this->filename = filename;
    frames.clear(); // 清除旧帧数据

    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, filename, nullptr, nullptr) != 0) {
        throw std::runtime_error("MP4 Init Cannot open the file");
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        avformat_close_input(&fmt_ctx);
        throw std::runtime_error("MP4 Init The stream information cannot be obtained");
    }

    int video_stream_idx = -1;
    int audio_stream_idx = -1;
    AVCodecParameters* video_codecpar = nullptr;
    AVCodecParameters* audio_codecpar = nullptr;

    // 查找视频流和音频流
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream* stream = fmt_ctx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            video_codecpar = stream->codecpar;
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            audio_codecpar = stream->codecpar;
        }
    }

    // 填充视频信息
    if (video_stream_idx >= 0) {
        video_width = video_codecpar->width;
        video_height = video_codecpar->height;

        AVStream* video_stream = fmt_ctx->streams[video_stream_idx];
        video_fps = (video_stream->avg_frame_rate.num && video_stream->avg_frame_rate.den)
            ? av_q2d(video_stream->avg_frame_rate) : -1;

        video_bitrate = video_codecpar->bit_rate > 0
            ? video_codecpar->bit_rate : -1;

        const AVCodec* codec = avcodec_find_decoder(video_codecpar->codec_id);
        video_codec_name = codec ? codec->name : "unknown";
    }

    // 填充音频信息
    if (audio_stream_idx >= 0) {
        audio_sample_rate = audio_codecpar->sample_rate;
        audio_channels = audio_codecpar->ch_layout.nb_channels;
        audio_bitrate = audio_codecpar->bit_rate > 0
            ? audio_codecpar->bit_rate : -1;

        const AVCodec* codec = avcodec_find_decoder(audio_codecpar->codec_id);
        audio_codec_name = codec ? codec->name : "unknown";
    }

    // 收集帧信息
    AVPacket pkt;
    av_init_packet(&pkt);
    frame_count = 0;

    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        if (pkt.stream_index == video_stream_idx) {
            MP4FrameInfo frame_info;
            frame_info.index = frame_count++;
            frame_info.size = pkt.size;
            frame_info.pts = pkt.pts;
            frame_info.dts = pkt.dts;
            frame_info.is_key_frame = (pkt.flags & AV_PKT_FLAG_KEY);
            frames.push_back(frame_info);
        }
        av_packet_unref(&pkt);
    }

    frame_size = frames.empty() ? -1 : static_cast<int>(frames[0].size);
    avformat_close_input(&fmt_ctx);
    }

    std::vector<MP4Frame> MP4::GetAllFramesVideo(MP4_Format* pcm_format) const {
            std::vector<MP4Frame> result;
    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* video_codec_ctx = nullptr, *audio_codec_ctx = nullptr;
    const AVCodec* video_codec = nullptr, *audio_codec = nullptr;
    int video_stream_idx = -1, audio_stream_idx = -1;
    SwsContext* sws_ctx = nullptr;
    SwrContext* swr_ctx = nullptr;

    // 打开文件
    if (avformat_open_input(&fmt_ctx, filename.c_str(), nullptr, nullptr) != 0) {
        throw std::runtime_error("MP4 GetAllFramesVideo Cannot open the file");
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        avformat_close_input(&fmt_ctx);
        throw std::runtime_error("MP4 GetAllFramesVideo The stream information cannot be obtained");
    }

    // 查找视频流和音频流
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream* stream = fmt_ctx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
        }
        else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
        }
    }

    if (video_stream_idx == -1) {
        avformat_close_input(&fmt_ctx);
        throw std::runtime_error("MP4 GetAllFramesVideo No video stream was found");
    }

    AVStream* video_stream = fmt_ctx->streams[video_stream_idx];
    AVStream* audio_stream = audio_stream_idx >= 0 ? fmt_ctx->streams[audio_stream_idx] : nullptr;

    // === 视频解码器初始化 ===
    AVCodecParameters* video_codecpar = fmt_ctx->streams[video_stream_idx]->codecpar;
    video_codec = avcodec_find_decoder(video_codecpar->codec_id);
    if (!video_codec) {
        avformat_close_input(&fmt_ctx);
        throw std::runtime_error("MP4 GetAllFramesVideo The video decoder cannot be found");
    }

    video_codec_ctx = avcodec_alloc_context3(video_codec);
    if (!video_codec_ctx) {
        avformat_close_input(&fmt_ctx);
        throw std::runtime_error("MP4 GetAllFramesVideo The video decoding context cannot be allocated");
    }

    if (avcodec_parameters_to_context(video_codec_ctx, video_codecpar) < 0) {
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        throw std::runtime_error("MP4 GetAllFramesVideo The video codec parameters cannot be copied");
    }

    if (avcodec_open2(video_codec_ctx, video_codec, nullptr) < 0) {
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        throw std::runtime_error("MP4 GetAllFramesVideo The video decoder cannot be opened");
    }
    // 设置输出采样格式
    enum AVSampleFormat out_sample_fmt = {};
    // 设置输出声道布局
    AVChannelLayout out_layout;
    // 使用视频流的时间基准作为参考
    AVRational common_time_base = video_stream->time_base;
    // 设置输出采样率
    int out_sample_rate;
    // 存储音频帧的队列
    std::queue<std::pair<int64_t, std::vector<uint8_t>>> audio_frame_queue;

    // === 音频解码器初始化 ===
    if (audio_stream_idx != -1) {
        AVCodecParameters* audio_codecpar = fmt_ctx->streams[audio_stream_idx]->codecpar;
        audio_codec = avcodec_find_decoder(audio_codecpar->codec_id);
        if (!audio_codec) {
            avcodec_free_context(&video_codec_ctx);
            avformat_close_input(&fmt_ctx);
            throw std::runtime_error("MP4 GetAllFramesVideo The audio decoder cannot be found");
        }

        audio_codec_ctx = avcodec_alloc_context3(audio_codec);
        if (!audio_codec_ctx) {
            avcodec_free_context(&video_codec_ctx);
            avformat_close_input(&fmt_ctx);
            throw std::runtime_error("MP4 GetAllFramesVideo The audio decoding context cannot be allocated");
        }

        if (avcodec_parameters_to_context(audio_codec_ctx, audio_codecpar) < 0) {
            avcodec_free_context(&audio_codec_ctx);
            avcodec_free_context(&video_codec_ctx);
            avformat_close_input(&fmt_ctx);
            throw std::runtime_error("MP4 GetAllFramesVideo The audio codec parameters cannot be copied");
        }

        if (avcodec_open2(audio_codec_ctx, audio_codec, nullptr) < 0) {
            avcodec_free_context(&audio_codec_ctx);
            avcodec_free_context(&video_codec_ctx);
            avformat_close_input(&fmt_ctx);
            throw std::runtime_error("MP4 GetAllFramesVideo The audio decoder cannot be opened");
        }

        // 初始化音频重采样器 (转换为PCM_S16)
        swr_ctx = swr_alloc();
        if (!swr_ctx) {
            avcodec_free_context(&audio_codec_ctx);
            avcodec_free_context(&video_codec_ctx);
            avformat_close_input(&fmt_ctx);
            throw std::runtime_error("MP4 GetAllFramesVideo Failed to allocate audio resampler");
        }

        // 设置输入声道布局
        AVChannelLayout in_layout;
        av_channel_layout_default(&in_layout, audio_codec_ctx->ch_layout.nb_channels);

        if (pcm_format) {
            // 根据pcm_format->Channels创建布局
            av_channel_layout_default(&out_layout, pcm_format->Channels);
        } else {
            // 默认立体声
            out_layout = AV_CHANNEL_LAYOUT_STEREO;
        }


        if (pcm_format) {
            // 使用指定的PCM格式
            out_sample_fmt = static_cast<AVSampleFormat>(pcm_format->format);
        } else {
            // 默认S16格式
            out_sample_fmt = AV_SAMPLE_FMT_S16;
        }


        if (pcm_format) {
            out_sample_rate = pcm_format->SamplingRate;
        } else {
            out_sample_rate = audio_codec_ctx->sample_rate;
        }

        // 配置重采样器
        int ret = swr_alloc_set_opts2(&swr_ctx,
                                     &out_layout,                // 输出声道布局
                                     out_sample_fmt,             // 输出采样格式
                                     out_sample_rate,            // 输出采样率
                                     &in_layout,                 // 输入声道布局
                                     audio_codec_ctx->sample_fmt,// 输入采样格式
                                     audio_codec_ctx->sample_rate,// 输入采样率
                                     0, nullptr);
        if (ret < 0 || swr_init(swr_ctx) < 0) {
            swr_free(&swr_ctx);
            avcodec_free_context(&audio_codec_ctx);
            avcodec_free_context(&video_codec_ctx);
            avformat_close_input(&fmt_ctx);
            throw std::runtime_error("MP4 GetAllFramesVideo Failed to initialize audio resampler");
        }
    }
    auto pix_fmt = AV_PIX_FMT_YUV420P;
    if (pcm_format) {
        pix_fmt = pcm_format->pix_fmt;
    }
    // 准备转换上下文（输出为指定像素格式）
    sws_ctx = sws_getContext(
        video_codec_ctx->width, video_codec_ctx->height, video_codec_ctx->pix_fmt,
        video_codec_ctx->width, video_codec_ctx->height, pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!sws_ctx) {
        if (audio_codec_ctx) avcodec_free_context(&audio_codec_ctx);
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        throw std::runtime_error("MP4 GetAllFramesVideo The conversion context cannot be initialized");
    }

    AVPacket* pkt = av_packet_alloc();
    AVFrame* video_frame = av_frame_alloc();
    AVFrame* audio_frame = av_frame_alloc();
    AVFrame* yuv_frame = av_frame_alloc();
    std::vector<uint8_t> yuv_buffer;
    std::vector<uint8_t> pcm_buffer;

    // 分配YUV缓冲区
    int yuv_size = av_image_get_buffer_size(pix_fmt,
                                           video_codec_ctx->width,
                                           video_codec_ctx->height,
                                           1);
    yuv_buffer.resize(yuv_size);
    av_image_fill_arrays(yuv_frame->data, yuv_frame->linesize,
                        yuv_buffer.data(), pix_fmt,
                        video_codec_ctx->width, video_codec_ctx->height, 1);

    // 读取并解码帧
    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == video_stream_idx) {
            if (avcodec_send_packet(video_codec_ctx, pkt) < 0) {
                av_packet_unref(pkt);
                continue;
            }

            while (avcodec_receive_frame(video_codec_ctx, video_frame) >= 0) {
                // 转换为指定像素格式
                sws_scale(sws_ctx,
                         (uint8_t const* const*)video_frame->data, video_frame->linesize,
                         0, video_codec_ctx->height,
                         yuv_frame->data, yuv_frame->linesize);

                // 创建MP4Frame对象并存储视频数据
                MP4Frame mp4_frame;
                mp4_frame.length = yuv_size;
                mp4_frame.data = yuv_buffer;

                int64_t video_pts = ConvertPts(video_stream, video_frame->pts, common_time_base);
                mp4_frame.pcm.pts = video_pts;

                // 计算此视频帧应包含的音频样本数
                double video_frame_duration = 1.0 / video_fps;
                int expected_samples = static_cast<int>(video_frame_duration * out_sample_rate);

                // 收集此视频帧对应的所有音频帧
                std::vector<uint8_t> combined_audio;
                while (!audio_frame_queue.empty()) {
                    auto& [audio_pts, audio_data] = audio_frame_queue.front();

                    if (audio_pts < video_pts + video_frame_duration) {
                        // 合并音频数据
                        combined_audio.insert(
                            combined_audio.end(),
                            audio_data.begin(),
                            audio_data.end()
                        );
                        audio_frame_queue.pop();
                    } else {
                        break;
                    }
                }

                // 分配给视频帧
                if (!combined_audio.empty()) {
                    mp4_frame.pcm.length = combined_audio.size();
                    mp4_frame.pcm.data = std::move(combined_audio);
                }

                result.push_back(std::move(mp4_frame));
            }
        }
        else if (pkt->stream_index == audio_stream_idx && audio_codec_ctx) {
            if (avcodec_send_packet(audio_codec_ctx, pkt) < 0) {
                av_packet_unref(pkt);
                continue;
            }

            while (avcodec_receive_frame(audio_codec_ctx, audio_frame) >= 0) {
                // 计算每个样本的字节数
                int bytes_per_sample = av_get_bytes_per_sample(out_sample_fmt);

                // 计算输出样本数
                int out_samples = swr_get_out_samples(swr_ctx, audio_frame->nb_samples);

                // 动态计算PCM缓冲区大小
                int pcm_size = out_samples * out_layout.nb_channels * bytes_per_sample;
                pcm_buffer.resize(pcm_size);

                uint8_t* out_data[1] = { pcm_buffer.data() };
                int actual_samples = swr_convert(swr_ctx, out_data, out_samples,
                                               (const uint8_t**)audio_frame->data, audio_frame->nb_samples);

                if (actual_samples > 0) {
                    // 计算实际PCM数据大小
                    int actual_size = actual_samples * out_layout.nb_channels * bytes_per_sample;

                    // 转换音频PTS到公共时间基准
                    int64_t audio_pts = ConvertPts(audio_stream, audio_frame->pts, common_time_base);

                    // 存储到队列而非立即分配
                    audio_frame_queue.emplace(audio_pts,
                        std::vector<uint8_t>(pcm_buffer.begin(), pcm_buffer.begin() + actual_size));
                }
            }
        }
        av_packet_unref(pkt);
    }
    for (auto& frame : result) {
        if (frame.pcm.length <= 0 && !audio_frame_queue.empty()) {
            auto& [audio_pts, audio_data] = audio_frame_queue.front();
            frame.pcm.length = audio_data.size();
            frame.pcm.data = std::move(audio_data);
            audio_frame_queue.pop();
        }
    }
    // 清理资源
    av_frame_free(&video_frame);
    av_frame_free(&audio_frame);
    av_frame_free(&yuv_frame);
    av_packet_free(&pkt);
    sws_freeContext(sws_ctx);
    if (swr_ctx) swr_free(&swr_ctx);
    avcodec_free_context(&video_codec_ctx);
    if (audio_codec_ctx) avcodec_free_context(&audio_codec_ctx);
    avformat_close_input(&fmt_ctx);

    return result;
    }


}


