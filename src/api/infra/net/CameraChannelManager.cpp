#include "api/infra/net/CameraChannelManager.h"
#include "api/common/Logger.h"
#include "api/infra/net/TcpCameraChannel.h"
#include <chrono>
#include <utility>

namespace api::infra {

CameraChannelManager::CameraChannelManager(std::shared_ptr<app::IIdAllocator> allocator,
                                           std::string account,
                                           std::string password,
                                           int registerTimeoutMs)
    : allocator_(std::move(allocator)),
      account_(std::move(account)),
      password_(std::move(password)),
      registerTimeoutMs_(registerTimeoutMs) {
}

CameraChannelManager::~CameraChannelManager() {
    Stop();
}

void CameraChannelManager::Start(int port) {
    serverSocket_ = new CTL::ServerSocket(port);
    serverSocket_->listen();
    acceptThread_ = std::thread([this] { AcceptLoop(); });
    LOG_INFO("[CameraChannelManager] TCP server on port %d", port);
}

void CameraChannelManager::Stop() {
    running_ = false;          // 先关开关，让 AcceptLoop 退出循环
    if (serverSocket_) {
        serverSocket_->close(); // 关闭监听，让阻塞中的 accept 返回 nullptr
    }
    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
    delete serverSocket_;
    serverSocket_ = nullptr;
    {
        std::unique_lock<std::shared_mutex> lock(sessionsMutex_);
        for (auto& [id, _] : sessions_) {
            allocator_->Release(id); // 会话销毁前先把 ID 还给池子
        }
        sessions_.clear();
    }
}

std::vector<int> CameraChannelManager::OnlineDevices() const {
    std::shared_lock<std::shared_mutex> lock(sessionsMutex_);
    std::vector<int> result;
    for (auto& [id, session] : sessions_) {
        if (session && session->IsOnline()) {
            result.push_back(id);
        }
    }
    return result;
}

std::shared_ptr<app::ICameraChannel> CameraChannelManager::Find(int deviceId) {
    std::shared_lock<std::shared_mutex> lock(sessionsMutex_);
    auto it = sessions_.find(deviceId);
    if (it != sessions_.end()) {
        return it->second;
    }
    return nullptr;
}

void CameraChannelManager::AcceptLoop() {
    while (running_) {
        CTL::ClientSocket* sock = serverSocket_->accept();
        if (!sock) {
            if (running_) continue; // 意外失败但服务还在，继续下一轮
            break;                  // 服务已停止，退出
        }

        auto session = std::make_shared<TcpCameraChannel>(
            *sock, allocator_, account_, password_);

        // 等待终端发 RequestID 完成注册（session 内部收到后调用 Allocate）
        auto deadline = std::chrono::steady_clock::now()
                      + std::chrono::milliseconds(registerTimeoutMs_);
        while (running_ && std::chrono::steady_clock::now() < deadline) {
            if (session->DeviceId() >= 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (!running_) break;
        if (session->DeviceId() < 0) {
            // 超时未注册：session 出作用域销毁，socket 随其析构关闭
            LOG_WARN("[CameraChannelManager] 注册超时，关闭连接");
            continue;
        }
        int id = session->DeviceId();
        LOG_INFO("[CameraChannelManager] 设备 %d 已连接", id);
        {
            std::unique_lock<std::shared_mutex> lock(sessionsMutex_);
            sessions_[id] = std::move(session);
        }
    }
}

} // namespace api::infra
