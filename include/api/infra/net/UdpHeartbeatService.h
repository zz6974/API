#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

namespace CTL {
class DatagramSocket;
} // namespace CTL

namespace api::infra {

/**
 * @brief UDP 心跳报文
 *
 * 固定布局，终端与服务端按同样的字节序对齐。
 */
struct HeartbeatPacket {
    char type[32];      ///< "HEARTBEAT" 或 "HEARTBEAT_ACK"
    uint32_t deviceId;  ///< 设备 ID
    uint64_t timestamp; ///< 发送时刻（毫秒），用于算 RTT
};

/**
 * @brief UDP 心跳服务（由旧 HeadBeat 平移改造）
 *
 * - 地址与端口由 Start 参数传入，不再硬编码
 * - socket 复用、ACK 打洞逻辑保持不变
 */
class UdpHeartbeatService {
public:
    UdpHeartbeatService();
    ~UdpHeartbeatService();

    /**
     * @brief 启动心跳服务
     * @param address 绑定地址
     * @param port 绑定端口
     * @param deviceId 起始心跳序号
     */
    void Start(const std::string& address, int port, int deviceId = 0);
    /** @brief 停止心跳服务 */
    void Stop();

private:
    /** @brief 周期广播心跳 + 等 ACK 完成打洞 */
    void SendLoop();
    /** @brief 收广播心跳并回应 ACK */
    void RecvLoop();
    /** @brief 组装并广播一个心跳包 */
    void SendHeartbeat();
    /**
     * @brief 处理收到的终端心跳
     * @param packet 心跳报文
     */
    void OnHeartbeatReceived(const HeartbeatPacket& packet);

    CTL::DatagramSocket* socket_ = nullptr; ///< UDP socket，BROADCAST 选项开广播
    std::thread sendThread_;                 ///< 发送线程
    std::thread recvThread_;                 ///< 接收线程
    std::atomic<bool> isRunning_{false};     ///< 运行开关
    std::atomic<int> deviceId_{0};          ///< 心跳自增序号（旧协议沿用的字段）
    int port_ = 0;                          ///< 对端（终端）监听端口
};

} // namespace api::infra
