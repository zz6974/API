# 摄像头终端（Camera Terminal）设计文档

## 概述

摄像头终端运行在 ARM Linux 设备上，负责将本地摄像头画面实时推流到 API 服务端。终端通过 HTTP 获取设备 ID，TCP 长连接接收指令和推送 JPEG 流，UDP 广播心跳保持在线。

### 技术栈

| 层 | 技术 |
|----|------|
| 图像采集 | OpenCV (VideoCapture) |
| 网络通信 | CC_SDK (ClientSocket, DatagramSocket, HttpClient) |
| 编码 | OpenCV imencode → JPEG |
| 编译 | CMake + 交叉编译工具链 (aarch64) |

### 项目结构

```
demo1/
├── include/
│   ├── Dispose.h              # 摄像头操作（打开/抓帧/编码）
│   ├── http.h                 # HTTP 客户端（登录/GetID）
│   ├── TcpCommandClient.h     # TCP 指令通道（8083 端口长连接）
│   ├── Headbeat.h             # UDP 心跳（8082 端口广播）
│   └── sock/                  # 旧网络封装（已弃用，保留兼容）
│       ├── client.h
│       ├── Socket.h
│       └── sys/sock.h
├── src/
│   ├── main.cpp               # 主入口
│   ├── Dispose.cpp
│   ├── http.cpp
│   ├── TcpCommandClient.cpp
│   ├── Headbeat.cpp
│   ├── URL.cpp                # 旧 HTTP 路由表（已弃用）
│   └── sock/
│       ├── client.cpp
│       └── Socket.cpp
├── CC_SDK/                    # CC_SDK 子模块
└── CMakeLists.txt
```

---

## 架构

```
main.cpp
  │
  ├─ Http ──────────────── 启动时调 2 次（登录 + GetID）
  │     └─ CTL::HttpClient（CC_SDK）
  │
  ├─ TcpCommandClient ──── 长连接，8083 端口
  │     ├─ RecvLoop 线程 ─ 持续收指令，放入队列
  │     └─ 主线程 ──────── WaitForCommand() 取指令
  │           ├─ SendStreamFrame() ─ 推流
  │           ├─ SendPhoto() ─────── 拍照回传
  │           └─ SendPong() ──────── TCP 保活回复
  │     └─ CTL::ClientSocket（CC_SDK）
  │
  ├─ HeadBeat ─────────── 两个后台线程，8082 端口
  │     ├─ SendLoop 线程 ─ 每 8 秒广播 HEARTBEAT
  │     └─ RecvLoop 线程 ─ 收 ACK，计算 RTT
  │     └─ CTL::DatagramSocket（CC_SDK）
  │
  └─ Dispose ──────────── 摄像头操作
        ├─ Open(index, config) ─ 设置分辨率/帧率
        ├─ GrabFrame() ───────── 返回 cv::Mat
        └─ EncodeJPEG(frame) ─── 返回 std::vector<uchar>
        └─ cv::VideoCapture（OpenCV）
```

---

## 通信协议

### TCP 8083 端口 — 指令通道

**终端 → 服务端**

| 消息 | 格式 | 说明 |
|------|------|------|
| 注册 | `REGISTER 3\n` | 连接后立即发送，告知设备 ID |
| 推流帧 | `STREAM_DATA 18432\n[18432 字节 JPEG]` | 先发文本头，紧接二进制 JPEG |
| 照片 | `PHOTO_DATA 9216\n[9216 字节 JPEG]` | 同上 |
| 保活回复 | `PONG\n` | 回复服务端的 PING |

**服务端 → 终端**

| 消息 | 格式 | 说明 |
|------|------|------|
| 开始推流 | `STREAM_START\n` | |
| 停止推流 | `STREAM_STOP\n` | |
| 拍照 | `TAKE_PHOTO\n` | |
| 保活探测 | `PING\n` | 终端应回复 PONG |
| 关机 | `SHUTDOWN\n` | 终端应优雅退出 |

**协议解析要点**

服务端发来的数据是连续的 TCP 字节流，例如：

```
STREAM_START\nSTREAM_STOP\nPING\n
```

终端用 `TcpCommandClient::ReadLine()` 逐行解析。关键实现：内部维护一个 4096 字节的读缓冲区 `readBuffer_`，一次 `recv` 读满缓冲区，然后逐字节消费直到遇到 `\n`。这样避免每次 `\n` 前的几十个字节都触发系统调用。

### UDP 8082 端口 — 心跳

**终端 → 服务端（广播）**

```
HEARTBEAT:<deviceId>:<seq>\n
```

每 8 秒发送一次，广播到 `192.168.1.172:8082`。

**服务端 → 终端（单播）**

```
HEARTBEAT_ACK:<deviceId>:<seq>\n
```

服务端收到后回复 ACK，终端据此计算 RTT 延迟。

### HTTP 8082 端口 — 管理接口

**POST /api/login**

```json
{"Account":"123456","Password":"123456"}
```

**GET /api/GetID**

```json
{"cam0":0}
```

---

## 模块详解

### 1. Dispose — 摄像头操作

```cpp
struct CameraConfig {
    int width       = 640;
    int height      = 480;
    int fps         = 30;
    int jpegQuality = 70;
};

class Dispose {
    bool Open(int index, const CameraConfig& cfg);
    void Close();
    cv::Mat GrabFrame();
    std::vector<uchar> EncodeJPEG(const cv::Mat& frame);
    bool IsOpen() const;
};
```

设计要点：
- 一个 Dispose 对象管理一个摄像头。多摄像头场景创建多个 Dispose 对象
- `cv::VideoCapture` 是栈对象（非指针），析构自动释放
- `Open()` 设置分辨率后会验证是否生效（打日志提示）

### 2. Http — HTTP 客户端

```cpp
class Http {
    bool Init(const std::string& serverIp, int port, ...);
    CTL::Result Get(const std::string& path);
    CTL::Result Post(const std::string& path, const std::string& body);
};
```

设计要点：
- 每次请求创建局部 `CTL::HttpClient`，用完即毁，不复用连接（终端 HTTP 请求极少）
- `Init()` 调用 `/api/login` 登录；`Get("/api/GetID")` 拿设备 ID

### 3. TcpCommandClient — TCP 指令通道

```cpp
class TcpCommandClient {
    bool Connect(const std::string& ip, int port, int deviceId);
    void Disconnect();
    std::string WaitForCommand(int timeoutMs);
    void SendStreamFrame(const std::vector<char>& jpeg);
    void SendPhoto(const std::vector<char>& jpeg);
    void SendPong();
};
```

设计要点：
- **双线程模型**：RecvLoop（后台线程）持续读 socket 收指令并放入队列；主线程调用 `WaitForCommand()` 从队列取指令，支持 timeout
- **SendStreamFrame / SendPhoto 共用实现**：`SendFrame(type, jpeg)` 发协议头 + 二进制体，两个公开接口只传不同 type 字符串
- **缓冲区读行**：`ReadLine()` 内部维护 4096 字节读缓冲，一次 `recv` 填满，逐字节消费找到 `\n`，大幅减少系统调用
- **析构顺序**：`running_=false → close() → join() → delete`，先关 socket 解除阻塞，再等线程退出

### 4. HeadBeat — UDP 心跳

```cpp
class Headbeat {
    void Start(int deviceId);
    void Stop();
    void SendLoop();   // 后台线程：每 8 秒广播 HEARTBEAT
    void RecvLoop();   // 后台线程：收 HEARTBEAT_ACK，计算 RTT
};
```

设计要点：
- 使用 CC_SDK 的 `CTL::DatagramSocket`，socket 在 `Start()` 创建一次，复用到 `Stop()`
- 发送前调用 `setSockOpt(BROADCAST)` 开启广播
- 服务端和终端的 HeadBeat 代码对称，仅 `OnHeartbeatReceived` 处理逻辑不同（服务端回复 ACK，终端计算 RTT）

### 5. main — 主入口

```
main 流程：
  1. 解析/加载 AppConfig
  2. Http::Init() 登录
  3. Http::Get("/api/GetID") 拿 deviceId
  4. TcpCommandClient::Connect() 连 8083
  5. Headbeat::Start() 启心跳
  6. Dispose::Open() 开摄像头
  7. 主循环：WaitForCommand(100ms) → 处理指令 → 推流 → 精确帧率控制
  8. 收到 SIGINT/SIGTERM 或 SHUTDOWN 指令 → 退出循环
  9. 清理：Close → Stop → Disconnect
```

关键配置结构体：

```cpp
struct AppConfig {
    std::string  serverIp    = "192.168.1.172";
    int          httpPort    = 8082;
    int          tcpPort     = 8083;
    int          udpPort     = 8082;
    int          cameraIndex = 0;
    CameraConfig camera;
};
```

---

## 端口分配

| 端口 | 协议 | 方向 | 用途 |
|------|------|------|------|
| 8082 | HTTP | 终端→服务端 | 登录、GetID |
| 8082 | UDP | 双向 | 心跳广播 + ACK |
| 8083 | TCP | 双向 | 指令下发 + 推流 + 拍照 |

---

## 编译

```bash
# ARM 交叉编译
cd demo1
mkdir -p build && cd build
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain_aarch64.cmake \
  -DOpenCV_DIR=/path/to/opencv/aarch64
make -j$(nproc)
```

CMakeLists.txt 关键项：

```cmake
add_subdirectory(CC_SDK)

add_executable(Camera
    src/main.cpp
    src/Dispose.cpp      include/Dispose.h
    src/http.cpp         include/http.h
    src/TcpCommandClient.cpp include/TcpCommandClient.h
    src/Headbeat.cpp     include/Headbeat.h
)

target_include_directories(Camera PUBLIC ${CC_API_INC})
target_link_libraries(Camera PUBLIC CC_API PRIVATE ${OpenCV_LIBS})
```

---

## 注意事项

1. **ReadLine 缓冲区残留**：收到 `\n` 后返回，缓冲区可能还有剩余字节。下次调用 ReadLine 时先消费残留，缓冲区空了再 recv
2. **SendStreamFrame 两次 send**：先发 `STREAM_DATA <size>\n`，再发 JPEG 二进制。服务端按 size 精确读取，中间不能多任何字节
3. **HeadBeat 与 PONG 的区别**：UDP 心跳是设备级保活（判断设备是否在线），TCP PING/PONG 是连接级保活（判断 TCP 管道是否畅通）
4. **nholmann::json 遍历**：`CTL::JSONObject` 继承自 `nlohmann::json`，遍历用 `items()` 而非 `.first/.second`：
   ```cpp
   for (auto& item : json.items()) {
       item.key();    // 键
       item.value();  // 值
   }
   ```
5. **已删除的模块**：`URL.h/URL.cpp` 是服务端的 HTTP 路由表，在摄像头终端中没有用处，已从 CMakeLists 移除
