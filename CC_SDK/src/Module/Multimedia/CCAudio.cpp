#include <iostream>
#include "CCAudio.h"
#include <CCThread.h>


bool CTL::Multimedia::Init()
{
    PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    return true;
}

void CTL::Multimedia::Release()
{
    Pa_Terminate();
}

unsigned int CTL::Multimedia::Audio::GetDeviceCount(){
    return Pa_GetDeviceCount();
}

std::vector<AudioDeviceInfo> CTL::Multimedia::Audio::GetDevices(const AudioType type){
    std::vector<AudioDeviceInfo> inputDevices;
    const auto deviceCount = GetDeviceCount();
    for (int i = 0; i < deviceCount; ++i){
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo){
            if(type == AudioType::Capture){
                if (deviceInfo->maxInputChannels > 0){
                    // 检查设备是否支持输入
                    AudioDeviceInfo device;
                    device.name = deviceInfo->name;
                    device.index = i;
                    device.Channels = deviceInfo->maxInputChannels;
                    device.deviceInfo = const_cast<PaDeviceInfo *>(deviceInfo);
                    // 检查是否已经存在相同名称和 hostApi 的设备
                    bool isDuplicate = false;
                    for (const auto& existingDevice : inputDevices) {
                        if (existingDevice.deviceInfo->hostApi == deviceInfo->hostApi) {
                            isDuplicate = true;
                            break;
                        }
                    }
                    if (!isDuplicate) {
                        inputDevices.push_back(device); // 将设备信息添加到数组中
                    }
                }
            }
            else if (type == AudioType::Render){
                if (deviceInfo->maxOutputChannels > 0){
                    AudioDeviceInfo device;
                    device.name = deviceInfo->name;
                    device.index = i;
                    device.Channels = deviceInfo->maxOutputChannels;
                    device.deviceInfo = const_cast<PaDeviceInfo*>(deviceInfo);
                    // 检查是否已经存在相同名称和 hostApi 的设备
                    bool isDuplicate = false;
                    for (const auto& existingDevice : inputDevices) {
                        if (existingDevice.deviceInfo->hostApi == deviceInfo->hostApi) {
                            isDuplicate = true;
                            break;
                        }
                    }
                    if (!isDuplicate) {
                        inputDevices.push_back(device); // 将设备信息添加到数组中
                    }
                }
            }
        }
    }
    return inputDevices;
}

std::vector<AudioDeviceInfo> CTL::Multimedia::Audio::GetDeviceAll(const AudioType type) {
    std::vector<AudioDeviceInfo> inputDevices; //用于存储输入设备的数组
    const auto deviceCount = GetDeviceCount();
    for (int i = 0; i < deviceCount; ++i){
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo){
            if(type == AudioType::Capture){
                if (deviceInfo->maxInputChannels > 0){
                    // 检查设备是否支持输入
                    AudioDeviceInfo device;
                    device.name = deviceInfo->name;
                    device.index = i;
                    device.Channels = deviceInfo->maxInputChannels;
                    device.deviceInfo = const_cast<PaDeviceInfo *>(deviceInfo);
                    inputDevices.push_back(device); // 将设备信息添加到数组中
                }
            }
            else if (type == AudioType::Render){
                if (deviceInfo->maxOutputChannels > 0){
                    AudioDeviceInfo device;
                    device.name = deviceInfo->name;
                    device.index = i;
                    device.Channels = deviceInfo->maxOutputChannels;
                    device.deviceInfo = const_cast<PaDeviceInfo*>(deviceInfo);
                    inputDevices.push_back(device); // 将设备信息添加到数组中
                }
            }
        }

    }
    return inputDevices;
}

CTL::Multimedia::StreamParameters CTL::Multimedia::Audio::GetParameters(const AudioDeviceInfo &info) {
    StreamParameters parameters;
    parameters.device = info.index;
    parameters.sampleFormat = paInt16;
    parameters.channelCount = info.Channels;
    parameters.suggestedLatency = info.deviceInfo->defaultLowInputLatency;
    parameters.hostApiSpecificStreamInfo = nullptr;
    return parameters;
}

bool CTL::Multimedia::Audio::SetStream(const StreamInfo* info){
    PaError err;
    if(Type == AudioType::Capture){
        err = Pa_OpenStream(&Stream,&Parameters, nullptr,
            info->sampleRate,info->framesPerBuffer,
            info->streamFlags,info->streamCallback,info->userData);
    }
    else{
        err = Pa_OpenStream(&Stream, nullptr, &Parameters,info->sampleRate,info->framesPerBuffer,
                            info->streamFlags,info->streamCallback,info->userData);
    }
    if (err != paNoError){
        error_ = err;
        std::cerr << "Failed to open record stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    return true;
}

void CTL::Multimedia::Audio::SetStreamParameters(const StreamParameters* parameters){
    Parameters.device = parameters->device;
    Parameters.channelCount = parameters->channelCount; // 录音单通道
    Parameters.sampleFormat = parameters->sampleFormat; // 32位浮点数样本
    Parameters.suggestedLatency = parameters->suggestedLatency;
    Parameters.hostApiSpecificStreamInfo = parameters->hostApiSpecificStreamInfo;
}

bool CTL::Multimedia::Audio::Start(const StreamInfo* info){
    if (info) {
        Stop();
        SetStream(info);
    }
    auto err = Pa_StartStream(Stream);
    if (err != paNoError) {
        error_ = err;
        std::cerr << "Failed to start stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    return true;
}

bool CTL::Multimedia::Audio::Stop(){
    if (Stream == nullptr) {
        return false;
    }
    const auto err = Pa_CloseStream(Stream);
    if (err != paNoError) {
        std::cerr << "Failed to start stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    Stream = nullptr;
    return true;
}

int CTL::Multimedia::Audio::GetDefaultDevice(const AudioType Type){
    if(Type == AudioType::Capture){
        return Pa_GetDefaultInputDevice();
    }
    else if(Type == AudioType::Render){
        return Pa_GetDefaultOutputDevice();
    }
    return -1;
}

bool CTL::Multimedia::Audio::ReadStream(void *buffer, unsigned long frames)
{
    auto err = Pa_ReadStream(Stream,buffer,frames);
    if(err == paNoError)
        return true;
    return false;
}

bool CTL::Multimedia::Audio::WriteStream(void *buffer, unsigned long frames)
{
    auto err = Pa_WriteStream(Stream,buffer,frames);
    if(err == paNoError)
        return true;
    return false;
}

CTL::String CTL::Multimedia::Audio::GetStreamError(){
    return CTL::String(Pa_GetErrorText(error_));
}

void CTL::Multimedia::Audio::SetSleep(long ms)
{
    Pa_Sleep(ms);
}

void CTL::Multimedia::Audio::SetMode(AudioType type)
{
    Type = type;
}

int CTL::Multimedia::Audio::CalculateLevenshteinDistance(const std::string &s1, const std::string &s2) {
    const size_t len1 = s1.size(), len2 = s2.size();
    std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1));

    for (size_t i = 0; i <= len1; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= len2; ++j) dp[0][j] = j;

    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            dp[i][j] = std::min({
                dp[i-1][j] + 1,    // 删除
                dp[i][j-1] + 1,    // 插入
                dp[i-1][j-1] + (s1[i-1] != s2[j-1]) // 替换
            });
        }
    }
    return dp[len1][len2];
}

bool CTL::Multimedia::Audio::IsDeviceNameSimilar(const std::string &name1, const std::string &name2, int threshold) {
    return CalculateLevenshteinDistance(name1, name2) <= threshold;
}

CTL::Multimedia::Audio::Audio(const AudioType type){
    Type = type;
}

CTL::Multimedia::Audio::~Audio(){
    Stop();
    Release();
    Stream = nullptr;
}


