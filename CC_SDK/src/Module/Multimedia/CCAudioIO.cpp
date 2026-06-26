#include "Multimedia/CCAudioIO.h"
#include "CCSystem.h"

#ifdef _WIN32
EXTERN_C const PROPERTYKEY DECLSPEC_SELECTANY PKEY_Device_FriendlyName =
    { { 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 } }, 14 };

EXTERN_C const PROPERTYKEY DECLSPEC_SELECTANY PKEY_Device_DeviceDesc =
    { { 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 } }, 2 };
#elif __linux__
#endif

#ifdef _WIN32
#elif __linux__
#endif

void CTL::AudioIO::Init() {
#ifdef _WIN32
    // const auto hr = CoInitialize(nullptr);
    // if (FAILED(hr)) {
    //     CTL::System::Println("AudioIO CoInitialize Error");
    // }
#elif __linux__
#endif
}

void CTL::AudioIO::Release() {
#ifdef _WIN32
    // CoUninitialize();
#elif __linux__
#endif
}

std::vector<CTL::AudioDevice> CTL::AudioIO::GetDeviceList(Audio_Type type) {
#ifdef _WIN32
    std::vector<AudioDevice> devices;

    if (type == Audio_Type::Render) {
        UINT deviceCount = waveOutGetNumDevs();
        for (UINT i = 0; i < deviceCount; i++) {
            WAVEOUTCAPS caps;
            MMRESULT result = waveOutGetDevCaps(i, &caps, sizeof(caps));

            if (result == MMSYSERR_NOERROR) {
                AudioDevice device;
                device.ID = i; // Wave API 使用设备索引而不是ID
                device.IsDefault = (i == WAVE_MAPPER);
                device.Name = wcharToString(caps.szPname);
                // Wave API 不提供详细描述
                device.Description = wcharToString(caps.szPname);
                devices.push_back(device);
            }
        }
        return devices;
    }
    else {
        // 获取输入设备数量
        UINT deviceCount = waveInGetNumDevs();
        for (UINT i = 0; i < deviceCount; i++) {
            WAVEINCAPS caps;
            MMRESULT result = waveInGetDevCaps(i, &caps, sizeof(caps));

            if (result == MMSYSERR_NOERROR) {
                AudioDevice device;
                device.ID = i; // Wave API 使用设备索引而不是ID
                device.IsDefault = (i == WAVE_MAPPER);
                device.Name = wcharToString(caps.szPname);
                // Wave API 不提供详细描述
                device.Description = wcharToString(caps.szPname);
                devices.push_back(device);
            }
        }
        return devices;
    }
#elif __linux__
    std::vector<AudioDevice> devices;

    if (type == Audio_Type::Render) {
        // 枚举播放设备
        snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
        snd_ctl_t *ctl;
        snd_pcm_info_t *pcminfo;
        snd_pcm_info_alloca(&pcminfo);

        // 获取默认播放设备名称
        std::string defaultDeviceName = "default";
        char *defaultPlaybackDevice = getenv("ALSA_PCM_CARD");
        if (defaultPlaybackDevice) {
            defaultDeviceName = std::string(defaultPlaybackDevice);
        }

        int card = -1;
        while (snd_card_next(&card) >= 0 && card >= 0) {
            char ctl_name[32];
            sprintf(ctl_name, "hw:%d", card);
            if (snd_ctl_open(&ctl, ctl_name, 0) < 0) {
                continue;
            }

            int dev = -1;
            while (snd_ctl_pcm_next_device(ctl, &dev) >= 0 && dev >= 0) {
                snd_pcm_info_set_device(pcminfo, dev);
                snd_pcm_info_set_subdevice(pcminfo, 0);
                snd_pcm_info_set_stream(pcminfo, stream);

                if (snd_ctl_pcm_info(ctl, pcminfo) >= 0) {
                    AudioDevice device;
                    char name[1024]{};
                    sprintf(name, "hw:%d,%d", card, dev);
                    device.ID = name;
                    char desc[64];
                    sprintf(desc, "Card %d, Device %d: %s", card, dev, snd_pcm_info_get_name(pcminfo));
                    device.Name = desc;

                    // 检查是否为默认设备
                    device.IsDefault = (strcmp(name, defaultDeviceName.c_str()) == 0);
                    // if (!device.IsDefault) {
                    //     // 也检查是否为card 0, device 0（通常为默认设备）
                    //     device.IsDefault = (card == 0 && dev == 0);
                    // }

                    devices.push_back(device);
                }
            }
            snd_ctl_close(ctl);
        }

        bool hasDefault = false;
        for (const auto& device : devices) {
            if (device.IsDefault) {
                hasDefault = true;
                break;
            }
        }
        if (!hasDefault && !devices.empty()) {
            AudioDevice device;
            device.ID = "default";
            device.Name = "Default Device";
            device.IsDefault = true;
            devices.push_back(device);
        }
    }
    else {
        // 枚举录音设备
        snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;
        snd_ctl_t *ctl;
        snd_pcm_info_t *pcminfo;
        snd_pcm_info_alloca(&pcminfo);

        // 获取默认录音设备名称
        std::string defaultDeviceName = "default";
        char *defaultCaptureDevice = getenv("ALSA_PCM_CARD");
        if (defaultCaptureDevice) {
            defaultDeviceName = std::string(defaultCaptureDevice);
        }

        int card = -1;
        while (snd_card_next(&card) >= 0 && card >= 0) {
            char ctl_name[32];
            sprintf(ctl_name, "hw:%d", card);
            if (snd_ctl_open(&ctl, ctl_name, 0) < 0) {
                continue;
            }

            int dev = -1;
            while (snd_ctl_pcm_next_device(ctl, &dev) >= 0 && dev >= 0) {
                snd_pcm_info_set_device(pcminfo, dev);
                snd_pcm_info_set_subdevice(pcminfo, 0);
                snd_pcm_info_set_stream(pcminfo, stream);

                if (snd_ctl_pcm_info(ctl, pcminfo) >= 0) {
                    AudioDevice device;
                    char name[1024]{};
                    sprintf(name, "hw:%d,%d", card, dev);
                    device.ID = name;
                    char desc[64];
                    sprintf(desc, "Card %d, Device %d: %s", card, dev, snd_pcm_info_get_name(pcminfo));
                    device.Name = desc;

                    // 检查是否为默认设备
                    device.IsDefault = (strcmp(name, defaultDeviceName.c_str()) == 0);
                    // if (!device.IsDefault) {
                    //     // 也检查是否为card 0, device 0（通常为默认设备）
                    //     device.IsDefault = (card == 0 && dev == 0);
                    // }

                    devices.push_back(device);
                }
            }
            snd_ctl_close(ctl);
        }
        bool hasDefault = false;
        for (const auto& device : devices) {
            if (device.IsDefault) {
                hasDefault = true;
                break;
            }
        }
        if (!hasDefault && !devices.empty()) {
            AudioDevice device;
            device.ID = "default";
            device.Name = "Default Device";
            device.IsDefault = true;
            devices.push_back(device);
        }
    }

    return devices;
#endif
}

bool CTL::AudioIO::Initialize(AudioParameter &param, Audio_Type type) {
    m_type = type;
    m_param = param;
#ifdef _WIN32
    if (type == Audio_Type::Capture) {
        if (m_isInitialized){
            return false;
        }
        m_param = param;

        // 设置音频格式
        m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        m_waveFormat.nChannels = static_cast<WORD>(param.channels);
        m_waveFormat.nSamplesPerSec = param.sampleRate;
        m_waveFormat.wBitsPerSample = static_cast<WORD>(param.bitsPerSample);
        m_waveFormat.nBlockAlign = (m_waveFormat.nChannels * m_waveFormat.wBitsPerSample) / 8;
        m_waveFormat.nAvgBytesPerSec = m_waveFormat.nSamplesPerSec * m_waveFormat.nBlockAlign;
        m_waveFormat.cbSize = 0;

        // 打开音频输入设备
        // 注意：Wave API 不直接支持通过ID选择设备，这里简化处理
        MMRESULT result = waveInOpen(&m_hWaveIn, m_param.device.ID, &m_waveFormat,
                                    reinterpret_cast<DWORD_PTR>(waveInProc),
                                    reinterpret_cast<DWORD_PTR>(this),
                                    CALLBACK_FUNCTION);

        if (result != MMSYSERR_NOERROR) {
            return false;
        }

        // 准备缓冲区
        m_buffers.resize(BUFFER_COUNT);
        m_waveHeaders.resize(BUFFER_COUNT);

        DWORD bufferSize = param.bufferFrames * m_waveFormat.nBlockAlign;

        for (int i = 0; i < BUFFER_COUNT; i++) {
            // 分配缓冲区内存
            m_buffers[i].resize(bufferSize);

            // 初始化波形头
            ZeroMemory(&m_waveHeaders[i], sizeof(WAVEHDR));
            m_waveHeaders[i].lpData = reinterpret_cast<LPSTR>(m_buffers[i].data());
            m_waveHeaders[i].dwBufferLength = bufferSize;

            // 准备缓冲区
            result = waveInPrepareHeader(m_hWaveIn, &m_waveHeaders[i], sizeof(WAVEHDR));
            if (result != MMSYSERR_NOERROR) {
                // 清理已分配的资源
                for (int j = 0; j < i; j++) {
                    waveInUnprepareHeader(m_hWaveIn, &m_waveHeaders[j], sizeof(WAVEHDR));
                }
                waveInClose(m_hWaveIn);
                m_hWaveIn = nullptr;
                return false;
            }

            // 添加缓冲区到设备
            result = waveInAddBuffer(m_hWaveIn, &m_waveHeaders[i], sizeof(WAVEHDR));
            if (result != MMSYSERR_NOERROR) {
                waveInUnprepareHeader(m_hWaveIn, &m_waveHeaders[i], sizeof(WAVEHDR));
                // 清理已分配的资源
                for (int j = 0; j < i; j++) {
                    waveInUnprepareHeader(m_hWaveIn, &m_waveHeaders[j], sizeof(WAVEHDR));
                }
                waveInClose(m_hWaveIn);
                m_hWaveIn = nullptr;
                return false;
            }
        }
        m_isInitialized = true;
        return true;
    }
    else {
        if (m_isPlayInitialized) {
            return false;
        }
        // 设置音频格式
        m_playWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
        m_playWaveFormat.nChannels = static_cast<WORD>(param.channels);
        m_playWaveFormat.nSamplesPerSec = param.sampleRate;
        m_playWaveFormat.wBitsPerSample = static_cast<WORD>(param.bitsPerSample);
        m_playWaveFormat.nBlockAlign = (m_playWaveFormat.nChannels * m_playWaveFormat.wBitsPerSample) / 8;
        m_playWaveFormat.nAvgBytesPerSec = m_playWaveFormat.nSamplesPerSec * m_playWaveFormat.nBlockAlign;
        m_playWaveFormat.cbSize = 0;

        // 打开音频输出设备
        MMRESULT result = waveOutOpen(&m_hWaveOut, param.device.ID, &m_playWaveFormat,
                                     reinterpret_cast<DWORD_PTR>(waveOutProc),
                                     reinterpret_cast<DWORD_PTR>(this),
                                     CALLBACK_FUNCTION);

        if (result != MMSYSERR_NOERROR) {
            return false;
        }

        // 准备缓冲区
        m_playBuffers.resize(PLAY_BUFFER_COUNT);
        m_playWaveHeaders.resize(PLAY_BUFFER_COUNT);

        DWORD bufferSize = param.bufferFrames * m_playWaveFormat.nBlockAlign;

        for (int i = 0; i < PLAY_BUFFER_COUNT; i++) {
            // 分配缓冲区内存
            m_playBuffers[i].resize(bufferSize);

            // 初始化波形头
            ZeroMemory(&m_playWaveHeaders[i], sizeof(WAVEHDR));
            m_playWaveHeaders[i].lpData = reinterpret_cast<LPSTR>(m_playBuffers[i].data());
            m_playWaveHeaders[i].dwBufferLength = bufferSize;

            // 准备缓冲区
            result = waveOutPrepareHeader(m_hWaveOut, &m_playWaveHeaders[i], sizeof(WAVEHDR));
            if (result != MMSYSERR_NOERROR) {
                // 清理已分配的资源
                for (int j = 0; j < i; j++) {
                    waveOutUnprepareHeader(m_hWaveOut, &m_playWaveHeaders[j], sizeof(WAVEHDR));
                }
                waveOutClose(m_hWaveOut);
                m_hWaveOut = nullptr;
                return false;
            }

            // 将缓冲区标记为可用
            m_availablePlayBuffers.push(i);
        }

        m_isPlayInitialized = true;
        return true;
    }
#elif __linux__
    // 参数验证
    if (param.channels == 0 || param.bitsPerSample == 0 || param.sampleRate == 0) {
        std::cerr << "Invalid audio parameters" << std::endl;
        return false;
    }

    if (param.bitsPerSample != 16) {
        std::cerr << "Currently only supports 16-bit audio" << std::endl;
        return false;
    }

    int err;
    snd_pcm_stream_t stream = (type == Capture) ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK;

    char device_name[64];
    if (strcmp(param.device.ID.c_str(), "default") == 0) {
        strcpy(device_name, "default");
    } else {
        strcpy(device_name, param.device.ID.c_str());
    }

    // 打开PCM设备
    std::cout << "尝试打开PCM设备: " << device_name << std::endl;
    if ((err = snd_pcm_open(&m_pcmHandle, device_name, stream, 0)) < 0) {
        std::cerr << "无法打开PCM设备 '" << device_name << "': " << snd_strerror(err) << std::endl;
        return false;
    }

    // 设置硬件参数
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);

    if ((err = snd_pcm_hw_params_any(m_pcmHandle, params)) < 0) {
        std::cerr << "Hardware parameters cannot be initialized: " << snd_strerror(err) << std::endl;
        snd_pcm_close(m_pcmHandle);
        return false;
    }

    // 设置访问类型
    if ((err = snd_pcm_hw_params_set_access(m_pcmHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        std::cerr << "You can't set the access type: " << snd_strerror(err) << std::endl;
        snd_pcm_close(m_pcmHandle);
        return false;
    }
    _snd_pcm_format format = {};
    if (param.bitsPerSample == 8) {
        format = SND_PCM_FORMAT_U8;
    }
    else if (param.bitsPerSample == 16) {
        format = SND_PCM_FORMAT_S16_LE;
    }
    else if (param.bitsPerSample == 24) {
        format = SND_PCM_FORMAT_S24_3LE;
    }
    else if (param.bitsPerSample == 32) {
        format = SND_PCM_FORMAT_S32_LE;
    }
    // 设置格式
    if ((err = snd_pcm_hw_params_set_format(m_pcmHandle, params, format)) < 0) {
        std::cerr << "Unable to set audio format: " << snd_strerror(err) << std::endl;
        snd_pcm_close(m_pcmHandle);
        return false;
    }
    unsigned int min_channels, max_channels;
    snd_pcm_hw_params_get_channels_min(params, &min_channels);
    snd_pcm_hw_params_get_channels_max(params, &max_channels);
    // 设置通道数（允许近似值）
    unsigned int channels = param.channels;
    if ((err = snd_pcm_hw_params_set_channels_near(m_pcmHandle, params, &channels)) < 0) {
        std::cerr << "The number of channels cannot be set: " << snd_strerror(err) << std::endl;
        snd_pcm_close(m_pcmHandle);
        return false;
    }

    if (channels != param.channels) {
        std::cout << "通道数调整为: " << channels << " (请求: " << param.channels << ")" << std::endl;
    }

    // 设置采样率（允许近似值）
    unsigned int rate = param.sampleRate;
    if ((err = snd_pcm_hw_params_set_rate_near(m_pcmHandle, params, &rate, 0)) < 0) {
        std::cerr << "The sample rate cannot be set: " << snd_strerror(err) << std::endl;
        snd_pcm_close(m_pcmHandle);
        return false;
    }

    if (rate != param.sampleRate) {
        std::cout << "The sampling rate is adjusted to: " << rate << " Hz (Request: " << param.sampleRate << " Hz)" << std::endl;
    }

    // 尝试设置周期大小（允许近似值）
    snd_pcm_uframes_t period_size = param.bufferFrames;

    // 设置周期大小
    if ((err = snd_pcm_hw_params_set_period_size_near(m_pcmHandle, params, &period_size, 0)) < 0) {
        std::cerr << "Unable to set the cycle size: " << snd_strerror(err) << std::endl;
        snd_pcm_close(m_pcmHandle);
        return false;
    }

    unsigned int periods = param.channels;
    // 设置周期数
    if ((err = snd_pcm_hw_params_set_periods_near(m_pcmHandle, params, &periods, 0)) < 0) {
        std::cerr << "The number of cycles cannot be set: " << snd_strerror(err) << std::endl;
        snd_pcm_close(m_pcmHandle);
        return false;
    }

    // 应用硬件参数
    if ((err = snd_pcm_hw_params(m_pcmHandle, params)) < 0) {
        std::cerr << "Hardware parameters cannot be applied: " << snd_strerror(err) << std::endl;
        snd_pcm_close(m_pcmHandle);
        return false;
    }

    // 获取实际的参数值
    snd_pcm_hw_params_get_period_size(params, &m_bufferFrames, 0);
    snd_pcm_hw_params_get_rate(params, &rate, 0);
    snd_pcm_hw_params_get_channels(params, &channels);

    m_param = param;
    m_param.sampleRate = rate; // 使用实际设置的采样率
    m_param.bufferFrames = m_bufferFrames; // 使用实际设置的周期大小
    m_param.channels = channels; // 使用实际设置的通道数
    m_isDeviceOpen = true;
    m_type = type;
    param = m_param;
    // std::cout << "音频设备初始化成功，设备: " << device_name
    //           << "，类型: " << (type == Capture ? "录音" : "播放")
    //           << "，采样率: " << rate << " Hz"
    //           << "，通道数: " << channels
    //           << "，周期大小: " << m_bufferFrames << " 帧" << std::endl;
    return true;
#endif
}

bool CTL::AudioIO::Start() {
#ifdef _WIN32
    if (m_type == Capture) {
        if (!m_isInitialized || m_isCapturing) {
            return false;
        }

        const MMRESULT result = waveInStart(m_hWaveIn);
        if (result == MMSYSERR_NOERROR) {
            m_isCapturing = true;
            return true;
        }
    }
    else {
        if (!m_isPlayInitialized || m_isPlaying) {
            return false;
        }

        MMRESULT result = waveOutRestart(m_hWaveOut);
        if (result == MMSYSERR_NOERROR) {
            m_isPlaying = true;
            return true;
        }
    }
    return false;
#elif __linux__
    if (!m_isDeviceOpen) return false;

    // 根据设备类型执行不同的启动操作
    if (m_type == Capture) {
        // 启动录音设备
        int err = snd_pcm_start(m_pcmHandle);
        if (err < 0) {
            std::cerr << "录音设备启动失败: " << snd_strerror(err) << std::endl;
            snd_pcm_recover(m_pcmHandle, err, 0);
            return false;
        }
        m_isCapturing = true;
        std::cout << "录音设备启动成功" << std::endl;
        return true;
    }
    else {
        // 播放设备启动需要准备设备
        int err = snd_pcm_prepare(m_pcmHandle);
        if (err < 0) {
            std::cerr << "播放设备准备失败: " << snd_strerror(err) << std::endl;
            snd_pcm_recover(m_pcmHandle, err, 0);
            return false;
        }
        m_isPlaying = true;
        std::cout << "播放设备准备成功" << std::endl;
        return true;
    }
#endif
}

void CTL::AudioIO::Stop() {
#ifdef _WIN32
    if (m_type == Capture) {
        if (!m_isInitialized) {
            return;
        }

        if (m_isCapturing) {
            waveInStop(m_hWaveIn);
            m_isCapturing = false;
        }

        // 重置设备以返回所有缓冲区
        waveInReset(m_hWaveIn);

        // 清理缓冲区
        for (int i = 0; i < BUFFER_COUNT; i++) {
            waveInUnprepareHeader(m_hWaveIn, &m_waveHeaders[i], sizeof(WAVEHDR));
        }

        // 关闭设备
        if (m_hWaveIn) {
            waveInClose(m_hWaveIn);
            m_hWaveIn = nullptr;
        }

        m_isInitialized = false;

        // 清空就绪缓冲区队列
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        while (!m_readyBuffers.empty()) {
            m_readyBuffers.pop();
        }
    }
    else {
        if (!m_isPlayInitialized) {
            return;
        }

        if (m_isPlaying) {
            waveOutPause(m_hWaveOut);
            m_isPlaying = false;
        }

        // 重置设备以停止所有播放
        waveOutReset(m_hWaveOut);

        // 清理缓冲区
        for (int i = 0; i < PLAY_BUFFER_COUNT; i++) {
            waveOutUnprepareHeader(m_hWaveOut, &m_playWaveHeaders[i], sizeof(WAVEHDR));
        }

        // 关闭设备
        if (m_hWaveOut) {
            waveOutClose(m_hWaveOut);
            m_hWaveOut = nullptr;
        }

        m_isPlayInitialized = false;

        // 清空可用缓冲区队列
        std::lock_guard<std::mutex> lock(m_playBufferMutex);
        while (!m_availablePlayBuffers.empty()) {
            m_availablePlayBuffers.pop();
        }
    }
#elif __linux__
    if (!m_isDeviceOpen) {
        return;
    }

    if (m_type == Capture) {
        // 停止录音设备
        if (m_isCapturing) {
            snd_pcm_drop(m_pcmHandle);
            m_isCapturing = false;
        }

        // 重置设备
        snd_pcm_reset(m_pcmHandle);

        // 关闭设备
        snd_pcm_close(m_pcmHandle);
        m_pcmHandle = nullptr;

        m_isDeviceOpen = false;
    }
    else {
        // 停止播放设备
        if (m_isPlaying) {
            snd_pcm_drain(m_pcmHandle);  // 等待播放完成
            m_isPlaying = false;
        }

        // 重置设备
        snd_pcm_reset(m_pcmHandle);

        // 关闭设备
        snd_pcm_close(m_pcmHandle);
        m_pcmHandle = nullptr;

        m_isDeviceOpen = false;
    }
#endif
}

bool CTL::AudioIO::WriteData(const ByteArray &buffer) {
#ifdef _WIN32
    if (!m_isPlayInitialized || !m_isPlaying) {
        return false;
    }

    std::unique_lock lock(m_playBufferMutex);

    // 等待有可用缓冲区（阻塞）
    while (m_availablePlayBuffers.empty() && m_isPlaying) {
        m_playBufferCondition.wait(lock);
    }

    // 如果停止播放，返回false
    if (!m_isPlaying && m_availablePlayBuffers.empty()) {
        return false;
    }

    // 获取可用的缓冲区索引
    int bufferIndex = m_availablePlayBuffers.front();
    m_availablePlayBuffers.pop();

    lock.unlock();

    // 复制数据到缓冲区
    WAVEHDR& waveHeader = m_playWaveHeaders[bufferIndex];
    DWORD copySize = std::min(static_cast<DWORD>(buffer.size()), waveHeader.dwBufferLength);
    memcpy(waveHeader.lpData, buffer.buffer(), copySize);
    waveHeader.dwBufferLength = copySize;

    // 播放缓冲区
    lock.lock();
    MMRESULT result = waveOutWrite(m_hWaveOut, &waveHeader, sizeof(WAVEHDR));
    lock.unlock();

    return (result == MMSYSERR_NOERROR);
#elif __linux__
    if (!m_isDeviceOpen) {
        std::cerr << "WriteData失败：设备未打开" << std::endl;
        return false;
    }

    if (m_type != Render) {
        std::cerr << "WriteData失败：设备类型不正确，期望Render，实际=" << m_type << std::endl;
        return false;
    }

    if (!m_isPlaying) {
        std::cerr << "WriteData失败：设备未处于播放状态" << std::endl;
        return false;
    }

    // 确保有数据需要播放
    if (buffer.size() == 0) {
        std::cerr << "WriteData失败：缓冲区为空" << std::endl;
        return false;
    }

    // 计算帧数
    size_t frameSize = (m_param.channels * m_param.bitsPerSample / 8);
    if (frameSize == 0) {
        std::cerr << "WriteData失败：帧大小计算错误" << std::endl;
        return false;
    }

    // 确保数据大小是帧大小的整数倍
    size_t dataSize = buffer.size();
    size_t frames = dataSize / frameSize;

    if (frames == 0) {
        std::cerr << "WriteData失败：没有完整帧数据" << std::endl;
        return false;
    }

    // 检查PCM状态
    snd_pcm_state_t state = snd_pcm_state(m_pcmHandle);
    if (state == SND_PCM_STATE_XRUN) {
        std::cout << "PCM处于XRUN状态，尝试恢复..." << std::endl;
        snd_pcm_recover(m_pcmHandle, -EPIPE, 0);
    }
    else if (state == SND_PCM_STATE_SUSPENDED) {
        std::cout << "PCM处于SUSPENDED状态，尝试恢复..." << std::endl;
        snd_pcm_recover(m_pcmHandle, -ESTRPIPE, 0);
    }

    // 写入音频数据
    snd_pcm_sframes_t frames_written = snd_pcm_writei(m_pcmHandle, buffer.buffer(), frames);

    if (frames_written < 0) {
        std::cerr << "PCM写入错误: " << snd_strerror(frames_written) << std::endl;
        // 尝试恢复错误
        int recover_err = snd_pcm_recover(m_pcmHandle, frames_written, 0);
        if (recover_err < 0) {
            std::cerr << "PCM恢复失败: " << snd_strerror(recover_err) << std::endl;
            return false;
        }
        return false;
    }

    // 检查写入的帧数是否正确
    if (frames_written != static_cast<snd_pcm_sframes_t>(frames)) {
        std::cerr << "写入帧数不匹配，期望: " << frames << "，实际: " << frames_written << std::endl;
    }

    return true;
#endif
}

bool CTL::AudioIO::ReadData(ByteArray &buffer) {
    buffer.resize(m_param.bufferFrames);
#ifdef _WIN32
    if (!m_isInitialized || !m_isCapturing) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_bufferMutex);

    // 等待有数据可用（阻塞）
    while (m_readyBuffers.empty() && m_isCapturing) {
        m_bufferCondition.wait(lock);
    }

    // 如果停止采集，返回false
    if (!m_isCapturing && m_readyBuffers.empty()) {
        return false;
    }

    // 获取就绪的缓冲区索引
    int bufferIndex = m_readyBuffers.front();
    m_readyBuffers.pop();

    lock.unlock();

    // 复制数据到输出缓冲区
    WAVEHDR& waveHeader = m_waveHeaders[bufferIndex];
    buffer.assign(reinterpret_cast<const uint8_t*>(waveHeader.lpData),
                  waveHeader.dwBytesRecorded);

    // 重新将缓冲区加入队列
    lock.lock();
    MMRESULT result = waveInAddBuffer(m_hWaveIn, &waveHeader, sizeof(WAVEHDR));
    lock.unlock();

    return (result == MMSYSERR_NOERROR);
#elif __linux__
    // 增加调试信息
    // std::cout << "ReadData调用 - 设备状态: " << (m_isDeviceOpen ? "已打开" : "未打开")
    //           << ", 类型: " << m_type << ", 是否采集: " << (m_isCapturing ? "是" : "否") << std::endl;

    if (!m_isDeviceOpen) {
        std::cerr << "ReadData失败：设备未打开" << std::endl;
        return false;
    }

    if (m_type != Capture) {
        std::cerr << "ReadData失败：设备类型不正确，期望Capture，实际=" << m_type << std::endl;
        return false;
    }

    if (!m_isCapturing) {
        std::cerr << "ReadData失败：设备未处于采集状态" << std::endl;
        return false;
    }

    // 检查缓冲区大小
    if (buffer.size() == 0) {
        std::cerr << "ReadData失败：缓冲区大小为0" << std::endl;
        return false;
    }

    // 计算帧大小
    size_t frameSize = (m_param.channels * m_param.bitsPerSample / 8);
    // std::cout << "帧大小计算: channels=" << m_param.channels
    //           << ", bitsPerSample=" << m_param.bitsPerSample
    //           << ", frameSize=" << frameSize << std::endl;

    if (frameSize == 0) {
        std::cerr << "ReadData失败：帧大小计算错误" << std::endl;
        return false;
    }

    size_t frames = buffer.size() / frameSize;
    // std::cout << "缓冲区大小: " << buffer.size() << ", 帧数: " << frames << std::endl;

    if (frames == 0) {
        std::cerr << "ReadData失败：没有完整帧数据" << std::endl;
        return false;
    }

    // 读取音频数据
    int err = snd_pcm_readi(m_pcmHandle, buffer.buffer(), frames);
    // std::cout << "snd_pcm_readi返回值: " << err << std::endl;

    if (err < 0) {
        std::cerr << "PCM读取错误: " << snd_strerror(err) << std::endl;
        // 尝试恢复错误
        const int recover_result = snd_pcm_recover(m_pcmHandle, err, 0);
        if (recover_result < 0) {
            std::cerr << "PCM恢复失败: " << snd_strerror(recover_result) << std::endl;
        }
        return false;
    }

    // 检查实际读取的帧数
    if (err != static_cast<int>(frames)) {
        std::cout << "实际读取帧数: " << err << "，期望帧数: " << frames << std::endl;
    }

    return true;
#endif
}

int CTL::AudioIO::GetDefaultDeviceIndex(const Audio_Type type) {
    const auto deviceList = GetDeviceList(type);
    for (size_t i = 0; i < deviceList.size(); ++i) {
        if (deviceList[i].IsDefault) {
            return static_cast<int>(i);
        }
    }
    return -1;
}
