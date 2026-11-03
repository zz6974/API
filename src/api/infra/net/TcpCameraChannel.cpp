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
    lastHeartbeat_ = std::chrono::steady_clock::now(); // 建连即记一次心跳，避免上线即超时
    recvThread_ = std::thread([this] { RecvLoop(); });
}

TcpCameraChannel::~TcpCameraChannel() {
    if (!socket_.isClosed()) {
        socket_.close(); // 先关 socket 让阻塞的 read 返回负值
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
    // 带谓词的定时等待：防虚假唤醒，也防 StorePhoto 在 wait 前抢先 notify
    if (!photoCv_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                           [this] { return photoReady_; })) {
        return common::Result<std::vector<char>>::Fail(
            common::ErrorCode::kTimeout, "拍照超时");
    }
    photoReady_ = false; // 消费掉结果，下一张照片重新等待
    return common::Result<std::vector<char>>::Ok(
        std::vector<char>(photoData_.begin(), photoData_.end()));
}

void TcpCameraChannel::RecvLoop() {
    char frame[4096];
    std::string lineBuffer;
    while (true) {
        int n = in_->read(frame, 4096);
        if (n < 0) break; // 连接断开（含析构时主动 close）

        for (int i = 0; i < n; i++) {
            if (binaryRemaining_ > 0) {
                // 二进制模式：逐字节吞进缓冲，直到收满声明长度
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
            // 行模式：攒到 \n 交给 ProcessLine，\r 直接丢弃
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

    // PONG：终端响应心跳探测，刷新心跳时间戳
    if (line.rfind(kPong, 0) == 0) {
        lastHeartbeat_ = std::chrono::steady_clock::now();
        return;
    }

    // RequestID：终端申请设备号，分配成功后回 ASSIGN 指令
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

    // LOGIN：校验账号密码（格式由旧协议约定），回 LOGIN_OK / LOGIN_FAIL
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

    // STREAMDATA N：预告 N 字节推流帧，切换状态机到二进制模式
    if (line.rfind(kStreamData, 0) == 0) {
        const char* p = line.c_str() + 11;
        if (*p == ' ') p++;
        binaryRemaining_ = std::stoull(p);
        binaryIsPhoto_ = false;
        binaryBuffer_.clear();
        binaryBuffer_.reserve(binaryRemaining_); // 预分配，避免逐字节追加反复扩容
        return;
    }

    // PHOTODATA N：预告 N 字节照片数据，二进制收满后归入照片而不是帧
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
    frameQueue_.Add(CTL::ByteArray(std::move(data))); // 只保留最新帧，队列有上限
}

void TcpCameraChannel::StorePhoto(std::vector<unsigned char>&& data) {
    std::unique_lock<std::mutex> lock(photoMutex_);
    photoData_ = std::move(data);
    photoReady_ = true;
    photoCv_.notify_one(); // 唤醒 TakePhoto 里等待的 HTTP 线程
}

} // namespace api::infra
