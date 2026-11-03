#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

namespace CTL {
class DatagramSocket;
} // namespace CTL

namespace api::infra {

// UDP 心跳报文
struct HeartbeatPacket {
    char type[32];
    uint32_t deviceId;
    uint64_t timestamp;
};

// UDP 心跳服务（由旧 HeadBeat 平移改造）：
// 1) 地址与端口由 Start 参数传入，不再硬编码
// 2) socket 复用、ACK 打洞逻辑保持不变
class UdpHeartbeatService {
public:
    UdpHeartbeatService();
    ~UdpHeartbeatService();

    void Start(const std::string& address, int port, int deviceId = 0);
    void Stop();

private:
    void SendLoop();
    void RecvLoop();
    void SendHeartbeat();
    void OnHeartbeatReceived(const HeartbeatPacket& packet);

    CTL::DatagramSocket* socket_ = nullptr;
    std::thread sendThread_;
    std::thread recvThread_;
    std::atomic<bool> isRunning_{false};
    std::atomic<int> deviceId_{0};
    int port_ = 0;
};

} // namespace api::infra
