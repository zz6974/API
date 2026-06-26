// include/CameraSession.h
#ifndef API_CAMERA_SESSION_H
#define API_CAMERA_SESSION_H

#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

#include "CCByteArray.h"
#include "CCServerSocket.h"
#include "CCWebSocket.h"
#include "TL/Queue.h"

/*
* 终端 → 服务端：
  REGISTER 3\n                           ← 告诉服务端我是设备 3
  STREAM_DATA 18432\n[18432 字节 JPEG]    ← 推流帧
  PHOTO_DATA 9216\n[9216 字节 JPEG]       ← 照片回传
  PONG\n                                  ← 心跳回复
服务端 → 终端：
  STREAM_START\n                          ← 开始推流
  STREAM_STOP\n                           ← 停止推流
  TAKE_PHOTO\n                            ← 拍照
  PING\n                                  ← 保活
 */

struct USER {
    std::string Account = "123456";
    std::string PassWord = "123456";
};

class CameraSession {
public:
    // 构造函数接收 accept 返回的 socket 指针
    // 注意：CameraSession 接管 socket 的所有权，析构时会 delete
    CameraSession(CTL::ClientSocket& sock);
    ~CameraSession();

    // 禁止拷贝（因为持有线程和 socket）
    CameraSession(const CameraSession&) = delete;
    CameraSession& operator=(const CameraSession&) = delete;

    // --- 查询接口 ---
    int  DeviceId() const { return deviceId_; }
    bool IsOnline() const;

    // --- 指令接口（线程安全，可在 HTTP 线程中调用） ---
    bool SendCommand(const std::string& cmd);
    CTL::ByteArray GetLatestFrame();
    CTL::ByteArray TakePhoto(int timeoutMs = 5000);

private:
    // 接收线程主循环
    void RecvLoop();

    // 协议解析
    void ProcessLine(const std::string& line);
    void ReadStreamData(size_t jpegSize);
    void ReadPhotoData(size_t jpegSize);
    int parseCommand(const std::string &line ,size_t  *pos);
    // 二进制数据读取
    //视频模式
    void StoreFrame(std::vector<unsigned char>&& data);
    //图片模式
    void StorePhoto(std::vector<unsigned char>&& data);

    CTL::ClientSocket socket_;
    CTL::SocketOutputStream *out = socket_.getOutputStream();
    CTL::SocketInputStream *in = socket_.getInputStream();
    std::thread recvThread_;
    int deviceId_ = -1;                    // -1 表示还没注册

    // 心跳超时判断
    std::chrono::steady_clock::time_point lastHeartbeat_;

    // 最新帧缓存（recvThread 写，HTTP 线程读）
    CTL::Queue<CTL::ByteArray> frameQueue_;

    // 拍照同步（HTTP 线程等，recvThread 通知）
    std::mutex photoMutex_;
    std::condition_variable photoCv_;
    std::vector<unsigned char> photoData_;
    bool photoReady_ = false;
    CTL::WebSocket websocket;// 视频流专属sock
    size_t binaryRemaining_ = 0;               // 0 = 行模式，>0 = 二进制模式
    std::vector<unsigned char> binaryBuffer_;  // 二进制累积区
    bool binaryIsPhoto_ = false;    // 二进制模式下，判断是否为照片还是图片
};


#endif
