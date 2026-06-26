#include "../include//CameraSession.h"
#include "CCClientSocket.h"
#include "CCRequest.h"
#include "HttpSocketChannel.h"
#include "../include/URL.h"
#include "../include/ID.h"
#include "CCSystem.h"
#include "opencv2/opencv.hpp"


CameraSession::CameraSession(CTL::ClientSocket &sock) : socket_(sock) {
    lastHeartbeat_ = std::chrono::steady_clock::now();
    recvThread_ = std::thread([this] { RecvLoop(); });
}

CameraSession::~CameraSession() {
    if (!socket_.isClosed()) {
        socket_.close();
    }
    if (recvThread_.joinable()) {
        recvThread_.join();
    }
}

bool CameraSession::IsOnline() const {
    if (deviceId_ < 0) return false;
    auto elapsed = std::chrono::steady_clock::now() - lastHeartbeat_;
    return elapsed < std::chrono::seconds(30);
}

//只是发送命令
bool CameraSession::SendCommand(const std::string &cmd) {
    bool F = out->write(cmd + "\n");
    if (F) {
        CTL::System::Println("[CameraSession] SendCommand: " + cmd);
    }
    return F;
}

//拿取视频流
CTL::ByteArray CameraSession::GetLatestFrame() {
    // if (frameQueue_.IsEmpty()) {
    //     CTL::System::Println("frameQueue无数据");
    // }
    auto result = frameQueue_.PollBack();
    if (!result) return {};
    // while (!frameQueue_.IsEmpty()) {
    //     frameQueue_.Poll();
    // }
    return *result;
}

//拿取照片
CTL::ByteArray CameraSession::TakePhoto(int timeoutMs) {
    SendCommand("TAKE_PHOTO");

    std::unique_lock<std::mutex> lock(photoMutex_);
    if (!photoCv_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                           [this] { return photoReady_; })) {
        return {};
                           }
    photoReady_ = false;
    return std::move(photoData_);
}

void CameraSession::RecvLoop() {
    char frame[4096];
    std::string lineBuffer;
    while (true) {
        int n = in -> read(frame, 4096);
        if (n < 0) break;

        for (int i = 0 ; i < n; i++ ) {
            if (binaryRemaining_ > 0) {
                binaryBuffer_.push_back(frame[i]);
                binaryRemaining_ --;
                if (binaryRemaining_ == 0) {
                    try {
                        if (binaryIsPhoto_) {
                            StorePhoto(std::move(binaryBuffer_));
                        }else {
                            StoreFrame(std::move(binaryBuffer_));
                        }
                    }catch (const std::exception &e) {
                        CTL::System::Println("[CameraSession] StoreFrame/Photo error: " + std::string(e.what()));
                    }
                    binaryBuffer_.clear();
                }
                continue;
            }
            char ch = frame[i];
            if (ch == '\n') {
                try {
                    ProcessLine(lineBuffer);
                }catch (const std::exception &e) {
                    CTL::System::Println("[CameraSession] ProcessLine error: " + std::string(e.what()));
                }
                lineBuffer.clear();
            }else if (ch != '\r') {
                lineBuffer.push_back(ch);
            }
        }
    }
}

void CameraSession::ProcessLine(const std::string &line) {
    if (line.empty()) return;

    if (line.rfind("PONG" , 0) == 0) {
        lastHeartbeat_ = std::chrono::steady_clock::now();
        return;
    }

    if (line.rfind("REQUEST_ID" , 0) == 0) {
        deviceId_ = GetID();
        SendCommand("ASSIGN_ID" + std::to_string(deviceId_));
        return;
    }

    if (line.rfind("LOGIN" , 0) == 0) {
        USER user;
        const char *numStart = line.c_str() + 5;
        std::string Account = line.substr(*numStart , line.find(""));
        std::string PassWord = line.substr(line.find(""));
        if (Account == user.Account && PassWord == user.PassWord) {
            SendCommand("LOGIN_OK");
        }else {
            SendCommand("LOGIN_FAIL");
        }
    }

    if (line.rfind("STREAM_DATA" , 0) == 0) {
        const char *p = line.c_str() + 11;
        if (*p == ' ') p++;
        binaryRemaining_ = std::stoull(p);
        binaryIsPhoto_ = false;
        binaryBuffer_.clear();
        binaryBuffer_.reserve(binaryRemaining_);
        return;
    }

    if (line.rfind("PHOTO_DATA", 0) == 0) {
        const char *numStart = line.c_str() + 10;
        if (*numStart == ' ') numStart++;
        binaryRemaining_ = std::stoull(numStart);
        binaryIsPhoto_   = true;
        binaryBuffer_.clear();
        binaryBuffer_.reserve(binaryRemaining_);
    }
}

void CameraSession::StoreFrame(std::vector<unsigned char> &&data) {
    frameQueue_.Add(CTL::ByteArray(std::move(data)));
}

void CameraSession::StorePhoto(std::vector<unsigned char> &&data) {
    std::unique_lock<std::mutex> lock(photoMutex_);
    photoData_ = std::move(data);
    photoReady_ = true;
    photoCv_.notify_one();
}
