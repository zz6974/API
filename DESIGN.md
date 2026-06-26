# 三端架构设计方案

## 整体拓扑

```
┌──────────────────┐
│   客户端 (http_pro) │  Qt6 桌面应用
│   192.168.1.x     │
└────────┬─────────┘
         │ HTTP (8082)
         ▼
┌──────────────────┐          TCP (8083)         ┌──────────────────┐
│   服务端 (API)     │◄─────────────────────────►│  摄像头终端 (demo1) │
│   192.168.1.172   │                            │  树莓派            │
│                   │   UDP 心跳 (8082)           │                   │
│   HTTP :8082      │◄──────────────────────────│  摄像头 + OpenCV   │
│   TCP  :8083      │                            │                   │
└──────────────────┘                            └──────────────────┘
```

端口分配：
- 8082：HTTP API + UDP 心跳
- 8083：TCP 指令通道（服务端 ↔ 摄像头终端）

---

## 一、服务端新增模块

### 1.1 文件结构

```
API/
├── include/
│   ├── CameraSession.h    # 单个摄像头 TCP 会话
│   ├── CameraManager.h    # 管理所有摄像头会话
│   ├── PhotoManager.h     # 照片存储 + 检索
│   └── HeadBeat.h         # 修复 + UDP 回复
├── src/
│   ├── CameraSession.cpp
│   ├── CameraManager.cpp
│   ├── PhotoManager.cpp
│   └── HeadBeat.cpp
├── photos/                # 照片存储目录（运行时创建）
└── CMakeLists.txt
```

### 1.2 CameraSession — 与一个摄像头终端的 TCP 长连接

```
每个摄像头终端在 TCP 8083 上建立一个长连接。
CameraSession 封装这一个连接，跑一个 recv 线程。
```

```cpp
// include/CameraSession.h
class CameraSession {
public:
    CameraSession(int deviceId, CTL::ClientSocket* sock);
    ~CameraSession();

    int  DeviceId() const;
    bool IsOnline() const;

    // 发送指令
    void SendCommand(const std::string& cmd);

    // 获取最新一帧（JPEG 字节）
    std::vector<uchar> GetLatestFrame() const;

    // 拍照：发送指令 → 等待照片数据 → 返回
    std::vector<uchar> TakePhoto(int timeoutMs = 5000);

private:
    void RecvLoop();        // 接收线程：解析协议帧
    void HandleStreamData(const std::vector<uchar>& jpeg);
    void HandlePhotoData(const std::vector<uchar>& jpeg);

    int  deviceId_;
    CTL::ClientSocket* socket_;         // 来自 accept
    std::thread recvThread_;
    std::atomic<bool> running_{true};

    // 最新帧缓存
    mutable std::mutex frameMutex_;
    std::vector<uchar> latestFrame_;

    // 拍照同步
    std::mutex photoMutex_;
    std::condition_variable photoCv_;
    std::vector<uchar> photoData_;
    bool photoReady_ = false;
};
```

**TCP 协议格式（文本头 + 二进制体）：**

```
# 摄像头 → 服务端
REGISTER <deviceId>\n
STREAM_DATA <jpegSize>\n<binary jpeg data>
PHOTO_DATA <jpegSize>\n<binary jpeg data>
PONG\n

# 服务端 → 摄像头
STREAM_START\n
STREAM_STOP\n
TAKE_PHOTO\n
PING\n
```

- 文本头以 `\n` 结束
- `STREAM_DATA` 和 `PHOTO_DATA` 后紧跟二进制 JPEG 数据，长度由 `<jpegSize>` 指定
- 简单、可读、易调试

### 1.3 CameraManager — 管理所有摄像头会话

```cpp
// include/CameraManager.h
class CameraManager {
public:
    static CameraManager& Instance();  // 单例

    void Start(int port = 8083);
    void Stop();

    // 查询
    std::vector<int> OnlineDevices() const;
    CameraSession* GetSession(int deviceId);

    // 指令（线程安全）
    void StartStream(int deviceId);
    void StopStream(int deviceId);
    std::vector<uchar> TakePhoto(int deviceId);
    std::vector<uchar> GetFrame(int deviceId);

private:
    void AcceptLoop();  // accept 线程

    CTL::ServerSocket* serverSocket_;
    std::thread acceptThread_;
    std::atomic<bool> running_{true};

    mutable std::shared_mutex sessionsMutex_;
    std::unordered_map<int, std::unique_ptr<CameraSession>> sessions_;
};
```

`CameraManager` 是单例，`main()` 中启动，生命周期与程序相同。`AcceptLoop` 在独立线程中阻塞 accept，每收到一个连接就创建 `CameraSession`。

### 1.4 PhotoManager — 照片存储

```cpp
// include/PhotoManager.h
class PhotoManager {
public:
    static PhotoManager& Instance();

    // 保存照片，返回唯一 photoId
    std::string SavePhoto(int deviceId, const std::vector<uchar>& jpeg);

    // 读取照片
    std::vector<uchar> GetPhoto(const std::string& photoId);

    // 列出某设备的照片
    std::vector<std::string> ListPhotos(int deviceId);

private:
    std::string photosDir_ = "./photos";
    std::mutex mutex_;
};
```

照片命名规则：`photos/{deviceId}_{timestamp}_{uuid}.jpg`。

### 1.5 HeadBeat 修复

```cpp
// 修复后的 HeadBeat.h
class Headbeat {
public:
    Headbeat();
    ~Headbeat();                        // 调用 Stop()

    void Start(int deviceId = 0);       // 传入设备 ID
    void Stop();

private:
    void SendLoop();
    void RecvLoop();
    void SendHeartbeat();
    void OnHeartbeatReceived(const HeartbeatPacket& packet);

    CTL::DatagramSocket* socket_ = nullptr;
    std::thread sendThread_;
    std::thread recvThread_;
    std::atomic<bool> isRunning{false};
    std::atomic<int>  deviceId_{0};
};
```

关键改动：
1. `isRunning` → `std::atomic<bool>`，线程安全退出
2. `int i` → `std::atomic<int> deviceId_`，由 `Start(deviceId)` 传入
3. socket 在 `Start()` 中创建一次，`Stop()` 中释放，不再每包泄漏
4. `OnHeartbeatReceived` 中**回复 ACK**：

```cpp
void Headbeat::OnHeartbeatReceived(const HeartbeatPacket& packet) {
    // ... 校验 + 打印 ...

    // UDP 打洞关键：回复 ACK
    HeartbeatPacket ack{};
    std::strncpy(ack.type, "HEARTBEAT_ACK", sizeof(ack.type));
    ack.deviceId = deviceId_;
    ack.timestamp = nowMs();

    CTL::ByteArray ackData(reinterpret_cast<const char*>(&ack), sizeof(ack));
    CTL::DatagramPacket ackPacket(1024);
    ackPacket.setData(ackData);
    ackPacket.setPort(Port);
    socket_->broadcast(ackPacket);       // 回复到同一端口，打洞成功
}
```

### 1.6 新增 HTTP 路由

```
POST /api/camera/{id}/stream/start   → CameraManager::StartStream(id)
POST /api/camera/{id}/stream/stop    → CameraManager::StopStream(id)
POST /api/camera/{id}/photo          → CameraManager::TakePhoto(id) → PhotoManager::SavePhoto() → 返回 photoId
GET  /api/camera/{id}/frame          → CameraManager::GetFrame(id) → 返回 JPEG 二进制
GET  /api/camera/list                → CameraManager::OnlineDevices() → 返回真正在线列表
GET  /api/photo/{photoId}            → PhotoManager::GetPhoto(photoId) → 返回 JPEG
GET  /api/photo/list/{deviceId}      → PhotoManager::ListPhotos(deviceId) → 返回 photoId 列表
```

`/api/camera/list` 替代原来的 `/api/camera/status`，数据来自 `CameraManager::OnlineDevices()`——从实际 TCP 连接判断在线状态。

### 1.7 main.cpp 改动

```cpp
int main() {
    system("chcp 65001");

    // 1. HTTP 服务
    CTL::WebServlet server;
    server.Init(Address, Port);
    Analysis analysis;
    analysis.Init();
    // ... 注册路由 ...

    // 2. TCP 指令通道
    CameraManager::Instance().Start(8083);

    // 3. UDP 心跳
    Headbeat heartbeat;
    heartbeat.Start();

    std::cerr << "[Server] HTTP :8082, TCP :8083, UDP :8082" << std::endl;
    server.Running();  // 阻塞
    return 0;
}
```

---

## 二、摄像头终端（demo1）改造

### 2.1 文件结构

```
demo1/
├── include/
│   ├── TcpCommandClient.h   # TCP 指令客户端（连接服务端 :8083）
│   ├── Dispose.h            # 摄像头控制（已有，需改）
│   └── HeadBeat.h           # 修复
├── src/
│   ├── TcpCommandClient.cpp
│   ├── Dispose.cpp
│   └── HeadBeat.cpp
└── main.cpp
```

### 2.2 TcpCommandClient — 连接服务端 TCP 指令通道

```cpp
// include/TcpCommandClient.h
class TcpCommandClient {
public:
    TcpCommandClient();
    ~TcpCommandClient();

    bool Connect(const std::string& serverIp, int port, int deviceId);
    void Disconnect();

    // 阻塞等待下一条指令
    std::string WaitForCommand(int timeoutMs = 0);  // 0 = 无限

    // 发送数据（推流 / 照片）
    void SendStreamFrame(const std::vector<uchar>& jpeg);
    void SendPhoto(const std::vector<uchar>& jpeg);

private:
    void RecvLoop();

    CTL::ClientSocket* socket_ = nullptr;
    std::thread recvThread_;
    std::atomic<bool> running_{false};
    int deviceId_;

    // 指令队列
    std::mutex cmdMutex_;
    std::condition_variable cmdCv_;
    std::queue<std::string> pendingCommands_;
};
```

### 2.3 Dispose 改造

`Dispose` 去掉 `HeadBeat` 相关逻辑（心跳移到 main），专注摄像头操作：

```cpp
class Dispose {
public:
    bool OpenCamera(int index = 0);
    void CloseCamera();
    cv::Mat GetFrame() const;
    std::vector<uchar> EncodeFrame(const cv::Mat& frame, int quality = 70);
    void ShowPreview(bool show);
    // 去掉 Init 中的 HeadBeat
};
```

### 2.4 main.cpp 改造 — 完整生命周期

```cpp
int main() {
    // 1. 申请 ID
    Http httpClient;
    httpClient.Demo_Init();                    // 登录
    CTL::Result result = httpClient.httpGet("/api/GetID");

    // 从 {"cam0":0} 解析 deviceId
    CTL::JSONObject json = CTL::JSONObject::parse(result.body);
    int deviceId = /* 从 json 中提取 */;

    // 2. 连接 TCP 指令通道
    TcpCommandClient tcpClient;
    tcpClient.Connect("192.168.1.172", 8083, deviceId);

    // 3. 打开摄像头
    Dispose dispose;
    dispose.OpenCamera(0);

    // 4. 启动心跳
    Headbeat heartbeat;
    heartbeat.Start(deviceId);

    // 5. 主循环：等待指令
    bool streaming = false;
    while (true) {
        std::string cmd = tcpClient.WaitForCommand(100);  // 100ms 超时，可检查退出

        if (cmd == "STREAM_START") {
            streaming = true;
        } else if (cmd == "STREAM_STOP") {
            streaming = false;
        } else if (cmd == "TAKE_PHOTO") {
            std::vector<uchar> jpeg = dispose.EncodeFrame(dispose.GetFrame());
            tcpClient.SendPhoto(jpeg);
        }

        // 如果正在推流，发送当前帧
        if (streaming) {
            cv::Mat frame = dispose.GetFrame();
            if (!frame.empty()) {
                std::vector<uchar> jpeg = dispose.EncodeFrame(frame);
                tcpClient.SendStreamFrame(jpeg);
            }
        }
    }

    heartbeat.Stop();
    tcpClient.Disconnect();
    return 0;
}
```

---

## 三、客户端（http_pro）改造

### 3.1 新增/修改文件

```
http_pro/
├── include/
│   ├── mainwindow.h     # 修改：加远程画面 + 拍照
│   └── http.h           # 修改：加新 API 封装
├── src/
│   ├── mainwindow.cpp
│   └── http.cpp
```

### 3.2 Http 类新增方法

```cpp
// 获取在线摄像头列表
CTL::Result GetCameraList();

// 获取指定摄像头的最新帧（JPEG）
CTL::Result GetCameraFrame(int deviceId);

// 控制推流
CTL::Result StartStream(int deviceId);
CTL::Result StopStream(int deviceId);

// 拍照
CTL::Result TakePhoto(int deviceId);        // 返回 {"photoId": "xxx"}
CTL::Result DownloadPhoto(const std::string& photoId, const std::string& savePath);
```

### 3.3 MainWindow 改造

```
┌─────────────────────────────────────────────┐
│  MainWindow                                  │
│                                              │
│  ┌──────────────┐  ┌──────────────────────┐ │
│  │  摄像头列表    │  │                      │ │
│  │              │  │                      │ │
│  │ ○ cam0 在线  │  │   实时画面 QLabel     │ │
│  │ ○ cam1 离线  │  │                      │ │
│  │ ○ cam2 在线  │  │   (定时器轮询         │ │
│  │              │  │    GET /frame)        │ │
│  │ [刷新列表]    │  │                      │ │
│  │              │  │                      │ │
│  │ [查看画面]    │  │                      │ │
│  │ [拍照]       │  └──────────────────────┘ │
│  │ [保存到...]  │                            │
│  └──────────────┘                            │
└─────────────────────────────────────────────┘
```

核心交互：

```
用户双击列表中的摄像头 → showCamera(id)
  → http->StartStream(id)          // 通知服务端开始推流
  → 启动 QTimer(33ms)              // ~30fps
  → 定时器回调：
      result = http->GetCameraFrame(id)
      QImage::fromData(result.body) → QPixmap → QLabel

用户点击 [拍照] → takePhoto(id)
  → result = http->TakePhoto(id)   // 返回 photoId
  → http->DownloadPhoto(photoId, savePath)
```

### 3.4 修复 URL::cameraStatus bug

```cpp
// 修复前（bug）：
result.body = req.RequestData.Body;   // 覆盖为空的！
CTL::JSONObject json = req.GetJson(); // 解析空内容

// 修复后：
CTL::JSONObject json = CTL::JSONObject::parse(result.body);
```

---

## 四、数据流完整路径

### 4.1 登录 → 查看列表

```
客户端                  服务端                 摄像头终端
  │                      │                      │
  │── POST /api/login ──→│                      │
  │←── 200 OK ──────────│                      │
  │                      │                      │
  │── GET /list ────────→│                      │
  │     (CameraManager   │                      │
  │      .OnlineDevices) │                      │
  │←── [cam0, cam2] ────│                      │
```

### 4.2 查看实时画面

```
客户端                  服务端                 摄像头终端
  │                      │                      │
  │── POST /stream/start→│                      │
  │                      │── TCP STREAM_START ─→│
  │                      │                      │ 开始编码发送
  │                      │←─ TCP STREAM_DATA ───│ (每 ~33ms 一帧)
  │                      │  缓存到 session      │
  │                      │                      │
  │── GET /frame ───────→│                      │
  │←── JPEG ────────────│  (返回缓存帧)         │
  │                      │                      │
  │ (定时器 30fps 轮询)   │                      │
```

### 4.3 拍照 + 下载

```
客户端                  服务端                 摄像头终端
  │                      │                      │
  │── POST /photo ──────→│                      │
  │                      │── TCP TAKE_PHOTO ───→│
  │                      │                      │ 拍照
  │                      │←─ TCP PHOTO_DATA ────│ (JPEG)
  │                      │  PhotoManager.Save   │
  │←── {photoId} ───────│                      │
  │                      │                      │
  │── GET /photo/{id} ──→│                      │
  │←── JPEG ────────────│                      │
  │  QFileDialog 保存    │                      │
```

### 4.4 UDP 心跳 + 打洞

```
服务端                 摄像头终端
  │                      │
  │←── UDP HEARTBEAT ───│  (广播)
  │  (记录来源 IP:Port)  │
  │── UDP ACK ─────────→│  (回复到来源地址，打洞成功)
  │                      │
  │  (如果 N 秒没收到心跳，标记离线)
```

---

## 五、实现优先级

| 优先级 | 模块 | 原因 |
|--------|------|------|
| P0 | 服务端 CameraSession + CameraManager | TCP 指令通道是一切的基础 |
| P0 | 摄像头终端 TcpCommandClient | 没有它终端收不到指令 |
| P0 | HeadBeat 修复（双端） | 当前代码有 use-after-free 和内存泄漏 |
| P1 | 服务端新增 HTTP 路由 | 客户端需要这些接口 |
| P1 | 客户端 MainWindow 改造 | 远程画面 + 拍照功能 |
| P2 | PhotoManager | 照片存储和检索 |
| P2 | 客户端 UI 优化 | 交互体验 |
