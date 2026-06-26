#ifndef CCAudio_H
#define CCAudio_H
#pragma once
#include "basic/CC.h"
#include "portaudio.h"


/**
 * 音频设备信息结构体
 */
struct AudioDeviceInfo {
    std::string name;
    unsigned int index;
    unsigned int Channels;
    PaDeviceInfo * deviceInfo = nullptr;
};

/**
 * 音频类型枚举
 */
enum AudioType
{
    Capture,
    Render
};

/**
 * 音频流和参数的类型定义
 */
typedef PaStream AudioStream;
typedef PaStreamParameters AudioParameter;

/**
 * CCMultimedia命名空间，包含音频处理相关类和函数
 */
namespace CTL::Multimedia {
    /**
     * 初始化音频系统
     * @return 成功返回true，否则返回false
     */
    bool Init();

    /**
     * 释放音频系统资源
     */
    void Release();

    struct StreamParameters {
        PaDeviceIndex device = 0;
        int channelCount = 1;
        PaSampleFormat sampleFormat = paInt16;
        PaTime suggestedLatency{};
        void *hostApiSpecificStreamInfo = nullptr;
    };
    struct StreamInfo {
        PaStreamCallback *streamCallback = nullptr;
        double sampleRate = 48000;
        unsigned long framesPerBuffer = 2048;
        PaStreamFlags streamFlags = paNoFlag;
        void *userData = nullptr;
    };
    enum Format {
        F32 = paFloat32,
        I16 = paInt16,
        I24 = paInt24,
        I32 = paInt32,
        U8 = paUInt8,
        I8 = paInt8,
        S8_U8 = paCustomFormat,
    };

    /**
     * CCAudio类，处理音频捕获和回放
     */
    class Audio
    {
        // 计算两个字符串的Levenshtein距离
        static int CalculateLevenshteinDistance(const std::string& s1, const std::string& s2);
        // 检查两个设备名称是否相似
        static bool IsDeviceNameSimilar(const std::string& name1, const std::string& name2, int threshold = 3);
    public:
        Audio() = default;

        /**
         * 构造函数，指定音频类型
         * @param type 音频类型，Capture或Render
         */
        explicit Audio(AudioType type);

        /**
         * 析构函数
         */
        ~Audio();

        /**
         * 打开音频流
         * @param info 流信息
         * @return 成功返回true，否则返回false
         */
        bool SetStream(const StreamInfo* info);

        /**
         * 启动音频流
         * @param info 流信息
         * @return 成功返回true，否则返回false
         */
        bool Start(const StreamInfo* info = nullptr);

        /**
         * 停止音频流
         * @return 成功返回true，否则返回false
         */
        bool Stop();

        /**
         * 从音频流读取数据
         * @param buffer 数据缓冲区
         * @param frames 帧数
         * @return 成功返回true，否则返回false
         */
        bool ReadStream(void *buffer,unsigned long frames);

        /**
         * 向音频流写入数据
         * @param buffer 数据缓冲区
         * @param frames 帧数
         * @return 成功返回true，否则返回false
         */
        bool WriteStream(void *buffer,unsigned long frames);

        /**
         * 获取音频流错误信息
         * @return 错误信息字符串
         */
        String GetStreamError();

        /**
         * 设置音频类型
         * @param Type 音频类型，Capture或Render
         */
        void SetMode(AudioType Type);

        /**
         * 设置睡眠时间
         * @param ms 睡眠时间，单位毫秒，默认50
         */
        void SetSleep(long ms = 50);

        /**
         * 设置音频流参数
         * @param parameters 参数
         */
        void SetStreamParameters(const StreamParameters* parameters);

        /**
         * 获取设备数量
         * @return 设备数量
         */
        static unsigned int GetDeviceCount();

        /**
         * 获取默认设备索引
         * @param Type 音频类型，Capture或Render
         * @return 默认设备索引
         */
        static int GetDefaultDevice(AudioType Type);

        /**
         * 获取过滤后的音频设备信息列表
         * @param type 音频类型，Capture或Render
         * @return 音频设备信息列表
         */
        static std::vector<AudioDeviceInfo> GetDevices(AudioType type);

        /**
         * 获取所有音频设备信息列表
         * @param type 音频类型，Capture或Render
         * @return 音频设备信息列表
         */
        static std::vector<AudioDeviceInfo> GetDeviceAll(AudioType type);

        /**
        * 快速初始化流参数
        * @param info 设备信息
        * @return StreamParameters 流参数
        */
        static StreamParameters GetParameters(const AudioDeviceInfo& info);

    private:
        PaStream *Stream{}; // 录音流和播放流
        PaStreamParameters Parameters{};
        AudioType Type = Render;
        bool BassStart = false;
        PaError error_{};
    };
}

#endif
