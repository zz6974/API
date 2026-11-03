#include "api/infra/net/TcpCameraChannel.h"
#include "api/common/Logger.h"
#include "api/domain/Protocol.h"
#include <utility>

namespace api::infra {

using namespace api::domain::protocol;

TcpCameraChannel::TcpCameraChannel(CTL::ClientSocket& sock,
                                   std::shared_ptr<app::IIdAllocator> allocator,
                                   std::string account,
                                   std::string password,
                                   int onlineTimeoutSeconds)
    : socket_(sock),
      allocator_(std::move(allocator)),
      account_(std::move(account)),
      password_(std::move(password)),
      onlineTimeoutSeconds_(onlineTimeoutSeconds) {
    out_ = socket_.getOutputStream();
    in_ = socket_.getInputStream();
    lastHeartbeat_ = std::chrono::steady_clock::now();
    recvThread_ = std::thread([this] { RecvLoop(); });
}

TcpCameraChannel::~TcpCameraChannel() {
    if (!socket_.isClosed()) {
        socket_.close();
    }
    if (recvThread_.joinable()) {
        recvThread_.join();
    }
}

bool TcpCameraChannel::IsOnline() const {
    if (deviceId_ < 0) return false;
    auto elapsed = std::chrono::steady_clock::now() - lastHeartbeat_;
    return elapsed < std::chrono::seconds(onlineTimeoutSeconds_);
}

void TcpCameraChannel::SendCommand(const std::string& command) {
    bool ok = out_->write(command + "\n");
    if (ok) {
        LOG_DEBUG("[TcpCameraChannel] SendCommand: %s", command.c_str());
    }
}

std::vector<char> TcpCameraChannel::GetLatestFrame() {
    auto result = frameQueue_.PollBack();
    if (!result) return {};
    const auto& frame = *result;
    return std::vector<char>(frame.begin(), frame.end());
}

common::Result<std::vector<char>> TcpCameraChannel::TakePhoto(int timeoutMs) {
    SendCommand(kTakePhoto);

    std::unique_lock<std::mutex> lock(photoMutex_);
    if (!photoCv_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                           [this] { return photoReady_; })) {
        return common::Result<std::vector<char>>::Fail(
            common::ErrorCode::kTimeout, "拍照超时");
    }
    photoReady_ = false;
    return common::Result<std::vector<char>>::Ok(
        std::vector<char>(photoData_.begin(), photoData_.end()));
}

void TcpCameraChannel::RecvLoop() {
    char frame[4096];
    std::string lineBuffer;
    while (true) {
        int n = in_->read(frame, 4096);
        if (n < 0) break;

        for (int i = 0; i < n; i++) {
            if (binaryRemaining_ > 0) {
                binaryBuffer_.push_back(frame[i]);
                binaryRemaining_--;
                if (binaryRemaining_ == 0) {
                    try {
                        if (binaryIsPhoto_) {
                            StorePhoto(std::move(binaryBuffer_));
                        } else {
                            StoreFrame(std::move(binaryBuffer_));
                        }
                    } catch (const std::exception& e) {
                        LOG_ERROR("[TcpCameraChannel] Store error: %s", e.what());
                    }
                    binaryBuffer_.clear();
                }
                continue;
            }
            char ch = frame[i];
            if (ch == '\n') {
                try {
                    ProcessLine(lineBuffer);
                } catch (const std::exception& e) {
                    LOG_ERROR("[TcpCameraChannel] ProcessLine error: %s", e.what());
                }
                lineBuffer.clear();
            } else if (ch != '\r') {
                lineBuffer.push_back(ch);
            }
        }
    }
}

void TcpCameraChannel::ProcessLine(const std::string& line) {
    if (line.empty()) return;

    if (line.rfind(kPong, 0) == 0) {
        lastHeartbeat_ = std::chrono::steady_clock::now();
        return;
    }

    if (line.rfind(kRequestId, 0) == 0) {
        auto idResult = allocator_->Allocate();
        if (idResult.IsOk()) {
            deviceId_ = idResult.Value();
            SendCommand(std::string(kAssignId) + std::to_string(deviceId_));
        } else {
            LOG_WARN("[TcpCameraChannel] ID 分配失败: %s", idResult.Message().c_str());
        }
        return;
    }

    if (line.rfind(kLogin, 0) == 0) {
        // 与旧实现保持一致：解析 LOGIN 行并回执
        const char* numStart = line.c_str() + 5;
        std::string account = line.substr(*numStart, line.find(""));
        std::string password = line.substr(line.find(""));
        if (account == account_ && password == password_) {
            SendCommand(kLoginOk);
        } else {
            SendCommand(kLoginFail);
        }
        return;
    }

    if (line.rfind(kStreamData, 0) == 0) {
        const char* p = line.c_str() + 11;
        if (*p == ' ') p++;
        binaryRemaining_ = std::stoull(p);
        binaryIsPhoto_ = false;
        binaryBuffer_.clear();
        binaryBuffer_.reserve(binaryRemaining_);
        return;
    }

    if (line.rfind(kPhotoData, 0) == 0) {
        const char* numStart = line.c_str() + 10;
        if (*numStart == ' ') numStart++;
        binaryRemaining_ = std::stoull(numStart);
        binaryIsPhoto_ = true;
        binaryBuffer_.clear();
        binaryBuffer_.reserve(binaryRemaining_);
    }
}

void TcpCameraChannel::StoreFrame(std::vector<unsigned char>&& data) {
    frameQueue_.Add(CTL::ByteArray(std::move(data)));
}

void TcpCameraChannel::StorePhoto(std::vector<unsigned char>&& data) {
    std::unique_lock<std::mutex> lock(photoMutex_);
    photoData_ = std::move(data);
    photoReady_ = true;
    photoCv_.notify_one();
}

} // namespace api::infra
