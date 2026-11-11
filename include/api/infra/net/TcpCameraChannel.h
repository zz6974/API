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

/**
 * @brief 与一个摄像头终端的 TCP 通道
 *
 * 实现 app::ICameraChannel 端口。
 * - ID 分配由注入的 IIdAllocator 提供
 * - TakePhoto 返回 Result<vector<char>>
 * - 协议常量统一用 api::domain::protocol
 */
class TcpCameraChannel : public app::ICameraChannel {
public:
    /**
     * @brief 构造（接管传入 socket 并立刻开始接收）
     * @param sock SDK 客户端 socket
     * @param allocator ID 分配器（端口接口）
     * @param account 终端登录账号
     * @param password 终端登录密码
     * @param onlineTimeoutSeconds 心跳在线判定超时（秒）
     */
    TcpCameraChannel(CTL::ClientSocket& sock,
                     std::shared_ptr<app::IIdAllocator> allocator,
                     std::string account,
                     std::string password,
                     int onlineTimeoutSeconds = 30);
    ~TcpCameraChannel() override;

    TcpCameraChannel(const TcpCameraChannel&) = delete;
    TcpCameraChannel& operator=(const TcpCameraChannel&) = delete;

    /**
     * @brief 设备 ID
     * @return 设备 ID，-1 表示还没注册
     */
    int DeviceId() const override { return deviceId_; }
    /**
     * @brief 是否在线
     * @return 距最后一次心跳不超过 30 秒
     */
    bool IsOnline() const override;
    /**
     * @brief 向终端发指令
     * @param command 指令内容（自动补 \n 换行）
     */
    void SendCommand(const std::string& command) override;
    /**
     * @brief 取最新帧
     * @return 队列尾部最新帧（无帧返回空）
     */
    std::vector<char> GetLatestFrame() override;
    /**
     * @brief 拍照并等待回传
     * @param timeoutMs 等待超时（毫秒）
     * @return 照片 JPEG；超时回 kTimeout
     */
    common::Result<std::vector<char>> TakePhoto(int timeoutMs) override;

private:
    /** @brief 收字节流，维护行/二进制状态机 */
    void RecvLoop();
    /**
     * @brief 行模式下按协议前缀分发
     * @param line 一行协议文本
     */
    void ProcessLine(const std::string& line);
    /** @brief 帧入队列（生产者侧） */
    void StoreFrame(std::vector<unsigned char>&& data);
    /** @brief 存照片并唤醒等待者 */
    void StorePhoto(std::vector<unsigned char>&& data);

    CTL::ClientSocket socket_;                 ///< 持有的终端连接
    CTL::SocketOutputStream* out_ = nullptr;   ///< 输出流（由 socket 提供）
    CTL::SocketInputStream* in_ = nullptr;     ///< 输入流（由 socket 提供）
    std::thread recvThread_;                   ///< 接收线程
    int deviceId_ = -1;                        ///< 设备 ID，-1 表示还没注册
    int onlineTimeoutSeconds_ = 30;            ///< 心跳在线判定超时（秒）

    /// 最后一次心跳时间（用于在线判定）
    std::chrono::steady_clock::time_point lastHeartbeat_;

    /// 最新帧缓存（recvThread 写，HTTP 线程读）
    CTL::Queue<CTL::ByteArray> frameQueue_;

    // 拍照同步（HTTP 线程等，recvThread 通知）
    std::mutex photoMutex_;                    ///< 拍照互斥锁
    std::condition_variable photoCv_;          ///< 拍照条件变量
    std::vector<unsigned char> photoData_;     ///< 照片数据
    bool photoReady_ = false;                  ///< 照片是否就绪

    size_t binaryRemaining_ = 0;               ///< 0 = 行模式，>0 = 二进制模式剩余字节数
    std::vector<unsigned char> binaryBuffer_;  ///< 二进制累积区
    bool binaryIsPhoto_ = false;               ///< 二进制是照片还是推流帧

    std::shared_ptr<app::IIdAllocator> allocator_; ///< ID 分配器（端口接口）
    std::string account_;                          ///< 终端登录账号
    std::string password_;                         ///< 终端登录密码
};

} // namespace api::infra
