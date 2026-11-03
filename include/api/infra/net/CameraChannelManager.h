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

// 摄像头通道池（由旧 CameraManager 平移改造）：
// 1) 实现 app::ICameraChannelManager 端口
// 2) sessions_ 由 unique_ptr 改为 shared_ptr（Find 返回 shared_ptr，
//    规避旧代码返回裸指针的悬垂风险）
// 3) ID 释放依赖注入的 IIdAllocator
class CameraChannelManager : public app::ICameraChannelManager {
public:
    CameraChannelManager(std::shared_ptr<app::IIdAllocator> allocator,
                         std::string account,
                         std::string password,
                         int registerTimeoutMs = 5000);
    ~CameraChannelManager() override;

    CameraChannelManager(const CameraChannelManager&) = delete;
    CameraChannelManager& operator=(const CameraChannelManager&) = delete;

    void Start(int port) override;
    void Stop() override;

    std::vector<int> OnlineDevices() const override;
    std::shared_ptr<app::ICameraChannel> Find(int deviceId) override;

private:
    void AcceptLoop();

    CTL::ServerSocket* serverSocket_ = nullptr;
    std::thread acceptThread_;
    std::atomic<bool> running_{true};

    mutable std::shared_mutex sessionsMutex_;
    std::unordered_map<int, std::shared_ptr<app::ICameraChannel>> sessions_;

    std::shared_ptr<app::IIdAllocator> allocator_;
    std::string account_;
    std::string password_;
    int registerTimeoutMs_;
};

} // namespace api::infra
