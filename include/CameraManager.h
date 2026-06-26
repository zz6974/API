// include/CameraManager.h
#ifndef API_CAMERA_MANAGER_H
#define API_CAMERA_MANAGER_H

#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <vector>

#include "CCServerSocket.h"

class CameraSession;

class CameraManager {
public:
    static CameraManager& Instance();   // 单例入口

    void Start(int port = 8083);
    void Stop();

    // --- 查询（读操作，shared_lock） ---
    std::vector<int> OnlineDevices() const;
    CameraSession*   GetSession(int deviceId) const;

    // --- 指令（内部找 session，发指令） ---
    std::vector<unsigned char> GetFrame(int deviceId) const;

    CTL::ByteArray TakePhoto(int deviceId) const;

    bool StartStream(int deviceId) const;

    bool StopStream(int deviceId) const;
    void                BroadcastData(std::string data) const;

    // --- 维护 ---
    void CleanupStaleSessions();
private:
    CameraManager() = default;
    ~CameraManager();

    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;

    void AcceptLoop();                  // accept 线程

    CTL::ServerSocket* serverSocket_ = nullptr;
    std::thread acceptThread_;
    std::atomic<bool> running_{true};

    // sessions_：deviceId → CameraSession
    mutable std::shared_mutex sessionsMutex_; // 读写锁
    std::unordered_map<int, std::unique_ptr<CameraSession>> sessions_; //容器，装载摄像头会话，一个摄像头一个
};

#endif