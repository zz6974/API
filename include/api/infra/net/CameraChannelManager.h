#pragma once
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "CCServerSocket.h"
#include "api/app/port/ICameraChannelManager.h"
#include "api/app/port/IIdAllocator.h"

namespace api::infra {

/**
 * @brief 摄像头通道池（由旧 CameraManager 平移改造）
 *
 * 实现 app::ICameraChannelManager 端口。
 * - sessions_ 由 unique_ptr 改为 shared_ptr（Find 返回 shared_ptr，规避旧代码返回裸指针的悬垂风险）
 * - ID 释放依赖注入的 IIdAllocator
 */
class CameraChannelManager : public app::ICameraChannelManager {
public:
    /**
     * @brief 构造
     * @param allocator ID 分配器（端口接口）
     * @param account 终端登录账号
     * @param password 终端登录密码
     * @param registerTimeoutMs 注册等待超时（毫秒）
     */
    CameraChannelManager(std::shared_ptr<app::IIdAllocator> allocator,
                         std::string account,
                         std::string password,
                         int registerTimeoutMs = 5000);
    ~CameraChannelManager() override;

    CameraChannelManager(const CameraChannelManager&) = delete;
    CameraChannelManager& operator=(const CameraChannelManager&) = delete;

    /**
     * @brief 开 TCP 监听，起 accept 线程
     * @param port 监听端口
     */
    void Start(int port) override;
    /** @brief 关监听、回收线程、释放全部 ID */
    void Stop() override;

    /**
     * @brief 在线设备列表
     * @return 读锁遍历，心跳未超时的才计入
     */
    std::vector<int> OnlineDevices() const override;
    /**
     * @brief 按 ID 查会话
     * @param deviceId 设备 ID
     * @return 会话指针，查不到返回 nullptr
     */
    std::shared_ptr<app::ICameraChannel> Find(int deviceId) override;

private:
    /** @brief 接受连接 -> 等注册 -> 入表 */
    void AcceptLoop();

    CTL::ServerSocket* serverSocket_ = nullptr; ///< SDK 对象，裸指针手动管理生命周期
    std::thread acceptThread_;                   ///< accept 线程
    std::atomic<bool> running_{true};           ///< 多线程开关，用原子避免数据竞争

    mutable std::shared_mutex sessionsMutex_;   ///< 读多写少：OnlineDevices/Find 共享锁，增删独占锁
    std::unordered_map<int, std::shared_ptr<app::ICameraChannel>> sessions_; ///< 设备 ID -> 会话

    std::shared_ptr<app::IIdAllocator> allocator_; ///< ID 池：注册时分配，断线时归还
    std::string account_;                          ///< 终端登录账号
    std::string password_;                         ///< 终端登录密码
    int registerTimeoutMs_;                     ///< 连接后等待终端注册的时限
};

} // namespace api::infra
