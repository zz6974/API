#ifndef CAMERA_HEADBEAT_H
#define CAMERA_HEADBEAT_H

#include "CCDatagramSocket.h"
#include "CCTimer.h"
#include <atomic>
#include <thread>

struct HeartbeatPacket {
    char type[32];
    uint32_t deviceId;
    uint64_t timestamp;
};

class Headbeat {
public:
    Headbeat();
    ~Headbeat();

    void Start(int deviceId = 0);
    void Stop();

private:
    void SendLoop();
    void RecvLoop();
    void SendHeartbeat();
    void OnHeartbeatReceived(const HeartbeatPacket& packet);

    CTL::DatagramSocket* socket_ = nullptr;
    std::thread sendThread_;
    std::thread recvThread_;
    std::atomic<bool> isRunning{false};
    std::atomic<int>  deviceId_{0};              // 替代原来的 int i
};

#endif