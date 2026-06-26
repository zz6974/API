#ifndef CC_AUDIO_IO_H
#define CC_AUDIO_IO_H

#include "vector"
#include "string"
#include "CCByteArray.h"

#ifdef _WIN32
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#pragma comment(lib, "ole32.lib")
#elif __linux__
#include <alsa/asoundlib.h>
#endif

#ifdef _WIN32
typedef UINT AudioID;
#elif __linux__
typedef std::string AudioID;
#endif

namespace CTL {
    enum Audio_Type {
        Capture = 0,
        Render
    };
    struct AudioDevice {
        AudioID ID{};
        bool IsDefault = false;
        std::string Name;
        std::string Description;
    };
    struct AudioParameter {
        unsigned int sampleRate = 44100;     // 采样率 (如 44100, 48000)
        unsigned int channels = 1;       // 通道数 (如 1=单声道, 2=立体声)
        unsigned int bitsPerSample = 16;  // 采样位数 (如 16, 24, 32)
        unsigned int bufferFrames = 1024;   // 缓冲区帧数
        AudioDevice device{};
    };
    class AudioIO{
        Audio_Type m_type = Capture;
        AudioParameter m_param;
#ifdef _WIN32
        HWAVEIN m_hWaveIn{};
        WAVEFORMATEX m_waveFormat{};
        std::vector<WAVEHDR> m_waveHeaders;
        std::vector<std::vector<BYTE>> m_buffers;
        std::mutex m_bufferMutex;
        std::condition_variable m_bufferCondition;
        std::queue<int> m_readyBuffers;
        bool m_isInitialized = false;
        bool m_isCapturing = false;
        static constexpr int BUFFER_COUNT = 3; // 双缓冲
        HWAVEOUT m_hWaveOut{};
        WAVEFORMATEX m_playWaveFormat{};
        std::vector<WAVEHDR> m_playWaveHeaders;
        std::vector<std::vector<BYTE>> m_playBuffers;
        std::mutex m_playBufferMutex;
        std::condition_variable m_playBufferCondition;
        std::queue<int> m_availablePlayBuffers;
        bool m_isPlayInitialized = false;
        bool m_isPlaying = false;
        static constexpr int PLAY_BUFFER_COUNT = 3;
#elif __linux__

#endif
    public:
    private:
#ifdef _WIN32
        static std::string wcharToString(const WCHAR* wstr) {
            if (!wstr) return std::string();
            int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
            std::string result(len - 1, 0);
            WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], len, nullptr, nullptr);
            return result;
        }
        static void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance,
                                   DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
            const auto pThis = reinterpret_cast<AudioIO*>(dwInstance);
            switch (uMsg) {
                case WIM_DATA:{
                    const auto pWaveHeader = reinterpret_cast<WAVEHDR*>(dwParam1);
                    // 找到完成的缓冲区索引
                    for (int i = 0; i < BUFFER_COUNT; i++) {
                        if (&pThis->m_waveHeaders[i] == pWaveHeader) {
                            // 将缓冲区标记为就绪
                            std::lock_guard<std::mutex> lock(pThis->m_bufferMutex);
                            pThis->m_readyBuffers.push(i);
                            pThis->m_bufferCondition.notify_one();
                            break;
                        }
                    }
                    break;
                }
                default: {
                    break;
                }
            }
        }
        static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
                                DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
            auto* pThis = reinterpret_cast<CTL::AudioIO*>(dwInstance);

            switch (uMsg) {
                case WOM_DONE:{
                    auto* pWaveHeader = reinterpret_cast<WAVEHDR*>(dwParam1);
                    // 找到完成的缓冲区索引
                    for (int i = 0; i < CTL::AudioIO::PLAY_BUFFER_COUNT; i++) {
                        if (&pThis->m_playWaveHeaders[i] == pWaveHeader) {
                            // 将缓冲区标记为可用
                            std::lock_guard<std::mutex> lock(pThis->m_playBufferMutex);
                            pThis->m_availablePlayBuffers.push(i);
                            pThis->m_playBufferCondition.notify_one();
                            break;
                        }
                    }
                    break;
                }
                default: {
                    break;
                }
            }
        }
#elif __linux__
        snd_pcm_t* m_pcmHandle{};
        snd_pcm_uframes_t m_bufferFrames = 1024;
        bool m_isDeviceOpen = false,m_isCapturing = false,m_isPlaying = false;
#endif
    public:
        //------------------------------------------------------------------------------------------
        static void Init();
        static void Release();
        //------------------------------------------------------------------------------------------
        static std::vector<AudioDevice> GetDeviceList(Audio_Type type);
        static int GetDefaultDeviceIndex(Audio_Type type);
        bool Initialize(AudioParameter& param, Audio_Type type);
        bool Start();
        void Stop();
        bool ReadData(ByteArray& buffer);
        bool WriteData(const ByteArray& buffer);
        //------------------------------------------------------------------------------------------
    };
}

#endif
