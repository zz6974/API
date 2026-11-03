#include "api/infra/net/UdpHeartbeatService.h"
#include "CCDatagramSocket.h"
#include "CCSystem.h"
#include "CCThread.h"
#include "api/common/Logger.h"
#include <cstring>

namespace api::infra {

namespace {

// 毫秒级时间戳（steady_clock 单调递增，适合测 RTT）
uint64_t NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

} // namespace

UdpHeartbeatService::UdpHeartbeatService() = default;

UdpHeartbeatService::~UdpHeartbeatService() {
    Stop();
}

void UdpHeartbeatService::Start(const std::string& address, int port, int deviceId) {
    deviceId_ = deviceId;
    port_ = port;

    socket_ = new CTL::DatagramSocket(port, CTL::InetAddress(address));
    socket_->setSockOpt(BROADCAST); // 开广播选项，心跳依赖子网广播发现

    isRunning_ = true;
    sendThread_ = std::thread([this] { SendLoop(); });
    recvThread_ = std::thread([this] { RecvLoop(); });
}

void UdpHeartbeatService::Stop() {
    isRunning_ = false; // 原子开关，两条循环各自退出

    if (sendThread_.joinable()) sendThread_.join();
    if (recvThread_.joinable()) recvThread_.join();

    delete socket_;
    socket_ = nullptr;
}

void UdpHeartbeatService::SendHeartbeat() {
    HeartbeatPacket packet{};
    std::strncpy(packet.type, "HEARTBEAT", sizeof(packet.type));
    packet.deviceId = deviceId_++; // 每次广播序号 +1，供对端区分新旧心跳
    packet.timestamp = NowMs();

    CTL::ByteArray data(reinterpret_cast<const char*>(&packet), sizeof(packet));
    CTL::DatagramPacket udpPacket(1024);
    udpPacket.setData(data);
    udpPacket.setPort(port_);
    socket_->broadcast(udpPacket);
}

void UdpHeartbeatService::SendLoop() {
    while (isRunning_) {
        SendHeartbeat();

        // 等一个 ACK（最多 3 秒）：收到即完成 UDP 打洞，对端后续可直连本机
        auto deadline = std::chrono::steady_clock::now()
                      + std::chrono::milliseconds(3000);
        while (isRunning_ && std::chrono::steady_clock::now() < deadline) {
            if (socket_->available()) {
                CTL::DatagramPacket packet(1024);
                socket_->receive(packet);
                break;
            }
            CTL::Thread::SleepMS(10); // 非阻塞轮询 + 小睡，避免忙等
        }

        // 休息 5 秒后进入下一轮心跳
        for (int i = 0; i < 50 && isRunning_; i++) {
            CTL::Thread::SleepMS(100);
        }
    }
}

void UdpHeartbeatService::RecvLoop() {
    while (isRunning_) {
        CTL::DatagramPacket packet(1024);
        try {
            socket_->receive(packet);
        } catch (...) {
            // 接收失败（如 socket 已关）短暂退避，避免异常风暴
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            continue;
        }

        if (packet.getLength() < sizeof(HeartbeatPacket))
            continue; // 残缺包直接忽略

        HeartbeatPacket pkt{};
        std::memcpy(&pkt, packet.getData(), sizeof(pkt));

        // 过滤 ACK 回复（ACK 由 SendLoop 处理）
        if (std::strncmp(pkt.type, "HEARTBEAT_ACK", sizeof(pkt.type)) == 0) {
            continue;
        }

        OnHeartbeatReceived(pkt);
    }
}

void UdpHeartbeatService::OnHeartbeatReceived(const HeartbeatPacket& packet) {
    if (std::strncmp(packet.type, "HEARTBEAT", sizeof(packet.type)) != 0) {
        LOG_WARN("[UdpHeartbeat] 未知包类型: %s", packet.type);
        return;
    }

    LOG_INFO("[UdpHeartbeat] 来自设备 %u 的心跳", packet.deviceId);
    uint64_t rtt = NowMs() - packet.timestamp;
    LOG_INFO("[UdpHeartbeat] RTT: %llu ms", static_cast<unsigned long long>(rtt));
    if (rtt > 5000) {
        LOG_WARN("[UdpHeartbeat] 设备 %u RTT %llu ms 过高",
                 packet.deviceId, static_cast<unsigned long long>(rtt));
    }

    // UDP 打洞：回复 ACK
    HeartbeatPacket ack{};
    std::strncpy(ack.type, "HEARTBEAT_ACK", sizeof(ack.type));
    ack.deviceId = deviceId_;
    ack.timestamp = NowMs();

    CTL::ByteArray ackData(reinterpret_cast<const char*>(&ack), sizeof(ack));
    CTL::DatagramPacket ackPkt(1024);
    ackPkt.setData(ackData);
    ackPkt.setPort(port_);
    socket_->broadcast(ackPkt);
}

} // namespace api::infra
