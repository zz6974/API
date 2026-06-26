#include "../include/CameraManager.h"
#include "../include/CameraSession.h"
#include "CCServerSocket.h"
#include "CCSystem.h"
#include "../include/ID.h"

CameraManager & CameraManager::Instance() {
    static CameraManager instance;
    return instance;
}

CameraManager::~CameraManager() {
    Stop();
}

void CameraManager::Start(int port) {
    serverSocket_ = new CTL::ServerSocket(port);
    serverSocket_->listen();
    acceptThread_ = std::thread([this]() {
        AcceptLoop();
    });
    CTL::System::Println("[CameraManager] TCP server on port {}", port);
}

void CameraManager::Stop() {
    running_ =  false;
    if (serverSocket_) {
        serverSocket_->close();
    }
    if (acceptThread_.joinable()) {
        acceptThread_ .join();
    }
    delete serverSocket_;
    serverSocket_ = nullptr;
    {
        std::unique_lock<std::shared_mutex> lock(sessionsMutex_);
        for (auto &[id, _] : sessions_) {
            ReleaseID(id);
        }
        sessions_.clear();
    }
}

std::vector<int> CameraManager::OnlineDevices() const {
    std::shared_lock<std::shared_mutex> lock (sessionsMutex_);
    std::vector<int> result;
    for (auto &[id , session] : sessions_) {
        if (session && session->IsOnline()) {
            result.push_back(id);
        }
    }
    return result;
}

CameraSession * CameraManager::GetSession(int deviceId) const {
    std::shared_lock<std::shared_mutex> lock(sessionsMutex_);
    auto it = sessions_.find(deviceId);
    if (it != sessions_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<unsigned char> CameraManager::GetFrame(int deviceId) const {
    auto* it = GetSession(deviceId);
    if ( it) {
        const auto frame =it ->GetLatestFrame();
        std::vector<unsigned char>  frame_t(frame.begin(), frame.end());
        return frame_t;
    }
    return {};
}

CTL::ByteArray CameraManager::TakePhoto(int deviceId) const {
    auto* it = GetSession(deviceId);
    if (it) {
        return it -> TakePhoto();
    }
    return {};
}

bool CameraManager::StartStream(int deviceId) const {
    auto* it = GetSession(deviceId);
    if (it) {
        return it ->SendCommand("STREAM_START");
    }
}

bool CameraManager::StopStream(int deviceId) const {
    auto* it = GetSession(deviceId);
    if (it) {
        return it ->SendCommand("STREAM_STOP");
    }
}

//广播数据
void CameraManager::BroadcastData(std::string data) const {
    std::shared_lock<std::shared_mutex> lock(sessionsMutex_);
    for (auto &[id, session] : sessions_) {
        if (session && session->IsOnline()) {
           session -> SendCommand(data);
        }
    }
}

void CameraManager::AcceptLoop() {
    while (running_) {
        CTL::ClientSocket *sock = serverSocket_->accept();
        if (!sock) {
            if (running_) continue;
            break;
        }

        auto session = std::make_unique<CameraSession>(*sock);

        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (running_ && std::chrono::steady_clock::now() < deadline) {
            if (session->DeviceId() >= 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        if (!running_) break;
        if (session -> DeviceId() < 0) {
            CTL::System::Println("[CameraManager] Accept timeout, closing");
            continue;
        }
        int id = session -> DeviceId();
        CTL::System::Println("[CameraManager] Device {} connected", id);
        {
            std::unique_lock<std::shared_mutex> lock(sessionsMutex_);
            sessions_[id] = std::move(session);
        }
    }
}


void CameraManager::CleanupStaleSessions() {
    std::unique_lock<std::shared_mutex> lock(sessionsMutex_);
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if (!it->second || !it->second->IsOnline()) {
            ReleaseID(it->first);
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}