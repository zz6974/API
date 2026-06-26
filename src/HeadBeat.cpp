#include <atomic>
#include <cstring>

#include "../include/HeadBeat.h"
#include "CCDatagramSocket.h"
#include "CCJSONObject.h"

#define Address "192.168.1.175"
#define Port 8082

// ============================================================
// 工具函数
// ============================================================

static uint64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

// ============================================================
// 生命周期
// ============================================================

Headbeat::Headbeat() {}

Headbeat::~Headbeat() {
    Stop();
}

// ============================================================
// Start / Stop
// ============================================================

void Headbeat::Start(int deviceId) {
    deviceId_ = deviceId;

    socket_ = new CTL::DatagramSocket(Port, CTL::InetAddress(Address));
    socket_->setSockOpt(BROADCAST);

    isRunning = true;
    sendThread_ = std::thread([this] { SendLoop(); });
    recvThread_ = std::thread([this] { RecvLoop(); });
}

void Headbeat::Stop() {
    isRunning = false;

    if (sendThread_.joinable()) sendThread_.join();
    if (recvThread_.joinable()) recvThread_.join();

    delete socket_;
    socket_ = nullptr;
}

// ============================================================
// 心跳发送
// ============================================================

void Headbeat::SendHeartbeat() {
    HeartbeatPacket packet{};
    std::strncpy(packet.type, "HEARTBEAT", sizeof(packet.type));
    packet.deviceId  = deviceId_++;
    packet.timestamp = nowMs();

    CTL::ByteArray data(reinterpret_cast<const char*>(&packet), sizeof(packet));
    CTL::DatagramPacket udpPacket(1024);
    udpPacket.setData(data);
    udpPacket.setPort(Port);
    socket_->broadcast(udpPacket);
}

void Headbeat::SendLoop() {
    while (isRunning) {
        SendHeartbeat();

        // 等待回复（最多 3 秒）
        auto deadline = std::chrono::steady_clock::now()
                      + std::chrono::milliseconds(3000);
        while (isRunning && std::chrono::steady_clock::now() < deadline) {
            if (socket_->available()) {
                CTL::DatagramPacket packet(1024);
                socket_->receive(packet);
                CTL::System::Println("Recv reply: {}",
                    packet.getDataAsString().c_str());
                break;
            }
            CTL::Thread::SleepMS(10);
        }

        // 休息 5 秒
        for (int i = 0; i < 50 && isRunning; i++) {
            CTL::Thread::SleepMS(100);
        }
    }
}

// ============================================================
// 心跳接收
// ============================================================

void Headbeat::RecvLoop() {
    while (isRunning) {
        CTL::DatagramPacket packet(1024);
        try {
            socket_->receive(packet);
        } catch (...) {
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            continue;
        }

        if (packet.getLength() < sizeof(HeartbeatPacket))
            continue;

        HeartbeatPacket pkt{};
        std::memcpy(&pkt, packet.getData(), sizeof(pkt));

        // 过滤掉 ACK 回复（ACK 由 SendLoop 处理，这里只收心跳请求）
        if (std::strncmp(pkt.type, "HEARTBEAT_ACK", sizeof(pkt.type)) == 0) {
            continue;
        }

        OnHeartbeatReceived(pkt);
    }
}

void Headbeat::OnHeartbeatReceived(const HeartbeatPacket& packet) {
    // 类型校验
    if (std::strncmp(packet.type, "HEARTBEAT", sizeof(packet.type)) != 0) {
        CTL::System::Println("Unknown packet type: {}", packet.type);
        return;
    }

    // 延迟统计
    CTL::System::Println("Heartbeat from device {}", packet.deviceId);
    uint64_t rtt = nowMs() - packet.timestamp;
    CTL::System::Println("RTT: {} ms", rtt);
    if (rtt > 5000) {
        CTL::System::Println("WARN: device {} rtt {} ms too high",
                             packet.deviceId, rtt);
    }

    // UDP 打洞：回复 ACK
    HeartbeatPacket ack{};
    std::strncpy(ack.type, "HEARTBEAT_ACK", sizeof(ack.type));
    ack.deviceId  = deviceId_;
    ack.timestamp = nowMs();

    CTL::ByteArray ackData(reinterpret_cast<const char*>(&ack), sizeof(ack));
    CTL::DatagramPacket ackPkt(1024);
    ackPkt.setData(ackData);
    ackPkt.setPort(Port);
    socket_->broadcast(ackPkt);
}
