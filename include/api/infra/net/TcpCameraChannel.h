#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "CCByteArray.h"
#include "CCClientSocket.h"
#include "TL/Queue.h"
#include "api/app/port/ICameraChannel.h"
#include "api/app/port/IIdAllocator.h"

namespace api::infra {

// 与一个摄像头终端的 TCP 通道（由旧 CameraSession 平移改造）：
// 1) 实现 app::ICameraChannel 端口
// 2) ID 分配由注入的 IIdAllocator 提供（替代全局 GetID）
// 3) TakePhoto 返回 Result<vector<char>>
// 4) 协议常量统一用 api::domain::protocol
class TcpCameraChannel : public app::ICameraChannel {
public:
    TcpCameraChannel(CTL::ClientSocket& sock,
                     std::shared_ptr<app::IIdAllocator> allocator,
                     std::string account,
                     std::string password,
                     int onlineTimeoutSeconds = 30);
    ~TcpCameraChannel() override;

    TcpCameraChannel(const TcpCameraChannel&) = delete;
    TcpCameraChannel& operator=(const TcpCameraChannel&) = delete;

    int DeviceId() const override { return deviceId_; }
    bool IsOnline() const override;
    void SendCommand(const std::string& command) override;
    std::vector<char> GetLatestFrame() override;
    common::Result<std::vector<char>> TakePhoto(int timeoutMs) override;

private:
    void RecvLoop();
    void ProcessLine(const std::string& line);
    void StoreFrame(std::vector<unsigned char>&& data);
    void StorePhoto(std::vector<unsigned char>&& data);

    CTL::ClientSocket socket_;
    CTL::SocketOutputStream* out_ = nullptr;
    CTL::SocketInputStream* in_ = nullptr;
    std::thread recvThread_;
    int deviceId_ = -1;                    // -1 表示还没注册
    int onlineTimeoutSeconds_ = 30;

    // 心跳超时判断
    std::chrono::steady_clock::time_point lastHeartbeat_;

    // 最新帧缓存（recvThread 写，HTTP 线程读）
    CTL::Queue<CTL::ByteArray> frameQueue_;

    // 拍照同步（HTTP 线程等，recvThread 通知）
    std::mutex photoMutex_;
    std::condition_variable photoCv_;
    std::vector<unsigned char> photoData_;
    bool photoReady_ = false;

    size_t binaryRemaining_ = 0;               // 0 = 行模式，>0 = 二进制模式
    std::vector<unsigned char> binaryBuffer_;  // 二进制累积区
    bool binaryIsPhoto_ = false;               // 二进制是照片还是推流帧

    std::shared_ptr<app::IIdAllocator> allocator_;
    std::string account_;
    std::string password_;
};

} // namespace api::infra
