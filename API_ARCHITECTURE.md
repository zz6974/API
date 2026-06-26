# API 服务端 — 完整逻辑文档

---

## 一、项目总览

### 1.1 这是什么

`API` 是一个 C++17 HTTP + TCP 服务器，运行在 `192.168.1.172` 上，是整个摄像头管理系统的中枢。

### 1.2 三个端口，三条通道

```
端口 8082 / HTTP   ← 客户端 (http_pro) 和 摄像头终端 (demo1) 通过 HTTP 请求服务
端口 8082 / UDP    ← 摄像头终端通过 UDP 发送心跳包
端口 8083 / TCP    ← 摄像头终端与服务端的 TCP 长连接，传指令、推流、拍照
```

### 1.3 文件清单

```
API/
├── CMakeLists.txt                    # 构建配置
├── include/
│   ├── Analysis.h                    # 请求分析器 — HTTP 入口
│   ├── URL.h                         # 路由表 — 路径 → 处理函数
│   ├── ID.h                          # ID 分配模块声明
│   ├── HeadBeat.h                    # UDP 心跳模块
│   ├── CameraSession.h               # 与一个摄像头终端的 TCP 会话
│   ├── CameraManager.h               # 所有摄像头会话的管理器（单例）
│   └── PhotoManager.h                # 照片存储检索（单例）
├── src/
│   ├── main.cpp                      # 程序入口
│   ├── Analysis.cpp                  # 请求分析器实现
│   ├── URL.cpp                       # 路由表实现 + 所有路由处理函数
│   ├── ID.cpp                        # ID 分配模块实现
│   ├── HeadBeat.cpp                  # UDP 心跳实现
│   ├── CameraSession.cpp             # TCP 会话实现
│   ├── CameraManager.cpp             # 会话管理器实现
│   └── PhotoManager.cpp              # 照片存储实现
├── CC_SDK/                           # 第三方 SDK（CMake 子项目）
├── photos/                           # 照片存储目录（运行时创建）
└── build/                            # 编译输出
```

### 1.4 类关系图

```
main()
  ├── Headbeat                    # UDP 心跳线程
  ├── Analysis                    # HTTP 请求入口
  │     └── URL                   # 路由表
  │           ├── HandleRegister  # POST /api/login
  │           ├── HandleID        # GET  /api/GetID
  │           ├── HandleCameraList      # GET  /api/camera/list
  │           ├── HandleCameraFrame     # GET  /api/camera/{id}/frame
  │           ├── HandleTakePhoto       # POST /api/camera/{id}/photo
  │           └── HandleStreamControl   # POST /api/camera/{id}/stream/start|stop
  │
  ├── CameraManager (单例)        # TCP 会话管理器
  │     └── CameraSession × N    # 每个在线摄像头一个
  │           ├── RecvLoop        # 收 TCP 数据（REGISTER/STREAM_DATA/PHOTO_DATA/PONG）
  │           ├── SendCommand     # 发 TCP 指令（STREAM_START/STOP/TAKE_PHOTO）
  │           ├── GetLatestFrame  # 取缓存帧（HTTP 线程调用）
  │           └── TakePhoto       # 拍照同步（HTTP 线程等待，TCP 线程通知）
  │
  └── PhotoManager (单例)         # 照片存储
        ├── SavePhoto             # 存盘 → 返回 photoId
        ├── GetPhoto              # 按 ID 读取
        └── ListPhotos            # 按设备 ID 列清单
```

### 1.5 三端通信全貌

```
客户端 (http_pro)                 服务端 (API)                  摄像头终端 (demo1)
     │                                │                               │
     │── POST /api/login ────────────→│                               │
     │←── 200 OK ─────────────────────│                               │
     │                                │                               │
     │── GET /api/camera/list ───────→│                               │
     │     └─ CameraManager::OnlineDevices()                          │
     │        └─ 遍历 sessions_ → IsOnline()（15秒心跳超时）            │
     │←── {"devices":[3],"count":1} ──│                               │
     │                                │                               │
     │                                │←── TCP REGISTER 3 ───────────│ (8083)
     │                                │                               │
     │── POST /api/camera/3/         │                               │
     │     stream/start ────────────→│                               │
     │     └─ CameraManager::StartStream(3)                           │
     │        └─ session->SendCommand("STREAM_START")                 │
     │                                │── TCP STREAM_START ──────────→│
     │                                │                               │
     │── GET /api/camera/3/frame ────→│                               │ 摄像头开始编码 OpenCV 帧
     │     └─ CameraManager::GetFrame(3)                              │
     │        └─ session->GetLatestFrame()                            │
     │           └─ lock(frameMutex_) → 返回缓存帧                    │
     │←── JPEG 二进制 ───────────────│                               │
     │                                │←── TCP STREAM_DATA 18KB ─────│ (推流帧)
     │                                │    → latestFrame_             │
     │                                │                               │
     │── POST /api/camera/3/photo ───→│                               │
     │     └─ CameraManager::TakePhoto(3)                             │
     │        └─ session->TakePhoto()                                 │
     │           ├─ SendCommand("TAKE_PHOTO")                         │
     │           └─ wait_for(5s) ← photoCv_                           │
     │                                │── TCP TAKE_PHOTO ────────────→│
     │                                │←── TCP PHOTO_DATA ────────────│
     │                                │    → photoData_ → notify      │
     │     └─ PhotoManager::SavePhoto(3, jpeg)                        │
     │        └─ 写文件 photos/3_xxx.jpg → 返回 photoId               │
     │←── {"photoId":"3_xxx.jpg"} ───│                               │
     │                                │                               │
     │                                │←── UDP HEARTBEAT ────────────│ (8082, 广播)
     │                                │── UDP HEARTBEAT_ACK ─────────→│ (打洞)
```

---

## 二、main.cpp — 程序启动

```
main()
  │
  ├─ system("chcp 65001")              // Windows 控制台 UTF-8
  │
  ├─ ① Headbeat heartbeat
  │     heartbeat.Start()              // 启动 UDP 心跳（见第九章）
  │       创建 socket
  │       启 sendThread: 每 ~8 秒广播心跳包 → 等 3 秒收回复
  │       启 recvThread: 持续阻塞收心跳、回复 ACK
  │
  ├─ ② Analysis analysis
  │     analysis.Init()                // 创建 URL → 注册 3 条精确路由（见第四章）
  │
  ├─ ③ CameraManager::Instance().Start(8083)   // 启动 TCP 指令通道（见第八章）
  │
  ├─ ④ server.Init("192.168.1.172", 8082)     // 启动 HTTP 服务器
  │
  │     注册精确路由（从 URL::Getroutes 获取）:
  │       /api/login       → 调用 analysis.HandleRequest
  │       /api/GetID       → 调用 analysis.HandleRequest
  │       /api/camera/list → 调用 analysis.HandleRequest
  │
  │     注册兜底函数 SetWebServletFun:
  │       所有未匹配的路径也调到 analysis.HandleRequest
  │       （带 deviceId 的路径如 /api/camera/3/frame 会走这里）
  │
  └─ server.Running()                  // 阻塞，等待 HTTP 请求
```

**设计要点：**

- `CameraManager::Start` 必须在 `server.Running` 之前调用，否则第一个 HTTP 请求到来时 TCP 通道还没打开。
- `Headbeat::Start` 放最前面是因为它独立运行，不影响其他模块。
- 两个注册方式（精确 + 兜底）的原因：
  - `SetWebServlet(path, handler)` 只在路径完全匹配时触发
  - `SetWebServletFun(handler)` 是兜底，所有未匹配的路径都会走到这里
  - `/api/camera/3/frame` 这种带设备 ID 的路径无法在启动时全部注册（因为 ID 是动态的），所以走兜底 → `HandleRequest` → `GetDeviceId` 提取 ID → 按 suffix 分发

---

## 三、Analysis — HTTP 请求入口

### 3.1 类定义 (include/Analysis.h)

```cpp
class Analysis {
public:
    // 生命周期
    Analysis();                         // url = nullptr
    ~Analysis();                        // delete url
    void Init();                        // new URL + url->Init()

    // 请求处理
    std::string HandleRequest(CTL::Request& req, int& status);  // ★ 入口
    std::string HandleGet(CTL::Request& req, int& status);      // GET 分发
    std::string HandlePost(CTL::Request& req, int& status);     // POST 分发

    // 路由查询
    std::map<std::string, RoutHandler> GetRoutes();

    // 辅助
    CTL::JSONObject httpParseBody(CTL::Request& req, int& status);
    static int GetDeviceId(std::string path, std::string& suffix);

private:
    URL* url;   // 路由表
};
```

### 3.2 HandleRequest — ★ 核心入口

每一个 HTTP 请求到达服务端后，最终都进入这个函数。它的处理流程分两步：

```
HandleRequest(req, status)
  │
  ├── 第一步：按 HTTP Method 分流
  │     GET  → HandleGet(req, status)
  │     POST → HandlePost(req, status)
  │     其他  → 返回 405
  │
  │     HandleGet / HandlePost 内部调用 url->Dispatch()
  │       精确匹配 routes map 中注册的路径:
  │         /api/login       → HandleRegister
  │         /api/GetID       → HandleID
  │         /api/camera/list → HandleCameraList
  │
  │     如果精确匹配命中（status != 404），直接返回
  │     如果未命中（status == 404），继续第二步
  │
  └── 第二步：带 deviceId 的前缀匹配
        GetDeviceId(path, suffix)  // 从 "/api/camera/3/frame" 提取:
                                     //   suffix="/frame", deviceId=3
        │
        ├─ suffix=="/frame"        → HandleCameraFrame     → CameraManager::GetFrame(deviceId)
        ├─ suffix=="/photo"        → HandleTakePhoto       → CameraManager::TakePhoto(deviceId)
        ├─ suffix=="/stream/start" → HandleStreamControl   → CameraManager::StartStream(deviceId)
        ├─ suffix=="/stream/stop"  → HandleStreamControl   → CameraManager::StopStream(deviceId)
        └─ 都不匹配 → 404
```

**设计要点：两步分发的必要性**

精确路由（注册在 URL::routes 里）只在路径完全匹配时生效。`/api/camera/3/frame` 中的 `3` 是动态的摄像头编号，无法启动时穷举注册，所以走 `GetDeviceId` 从路径中实时提取。

两步的顺序不能颠倒：先精确匹配（`/api/login`、`/api/GetID`），再前缀匹配（`/api/camera/*`）。因为 `/api/camera/list` 是精确路径，必须第一步命中而不是被前缀逻辑误吞。

### 3.3 HandleGet

```cpp
std::string HandleGet(CTL::Request& req, int& status) {
    std::string result;
    if (url && url->Dispatch(req.RequestData.Path, req, result, status)) {
        return result;        // 精确路由命中
    }
    status = 404;             // 未命中，返回 404
    return R"({"code":404,"msg":"未找到路由"})";
}
```

### 3.4 HandlePost

```cpp
std::string HandlePost(CTL::Request& req, int& status) {
    std::string result;
    if (url && url->Dispatch(req.RequestData.Path, req, result, status)) {
        return result;        // 精确路由命中
    }
    status = 404;             // 未命中，返回 404
    return R"({"code":404,"msg":"未找到路由"})";
}
```

**注意：** HandlePost 不再强制要求 Body 不为空。需要 Body 的路由（如 `HandleRegister`）自己内部校验。

### 3.5 GetDeviceId — 路径解析工具

```
输入: path="/api/camera/3/frame"
      suffix（传出参数）

内部:
  start = path.find("/")           // 找到第一个 "/" 的位置
  end   = path.find("/", start+1)  // 找到第二个 "/" 的位置

  如果 end == npos（无后缀，如 "/api/camera/3"）:
    suffix = ""
    取 path.substr(start+1) → "api/camera/3" → stoi → 返回 -1 或数字

  如果有后缀（如 "/api/camera/3/frame"）:
    idStr  = path.substr(start+1, end-start-1)  // "api/camera/3" 中取出一段
    suffix = path.substr(end)                    // "/frame"
    stoi(idStr 中最后一段数字) → 返回 3
```

---

## 四、URL — 路由表

### 4.1 类定义 (include/URL.h)

```cpp
using RoutHandler = std::function<std::string(CTL::Request&, int&)>;

class URL {
public:
    void Init();                                    // 注册 3 条精确路由
    void add(const std::string& path, RoutHandler); // 添加一条路由
    bool Dispatch(const std::string& path, CTL::Request&, std::string&, int&);

    // 6 个路由处理函数（全部 static，因为不依赖 URL 实例状态）
    static std::string HandleRegister(CTL::Request&, int&);
    static std::string HandleID(CTL::Request&, int&);
    static std::string HandleCameraList(CTL::Request&, int&);
    static std::string HandleCameraFrame(CTL::Request&, int&);
    static std::string HandleTakePhoto(CTL::Request&, int&);
    static std::string HandleStreamControl(CTL::Request&, int&);

private:
    std::map<std::string, RoutHandler> routes;  // 路径 → 处理函数
    static CCVector<int> all_id;                // 已分配的 ID 列表
    static std::mutex containerMutex;           // 保护 all_id
};
```

### 4.2 Init — 路由注册

```cpp
void Init() {
    add("/api/login",       HandleRegister);
    add("/api/GetID",       HandleID);
    add("/api/camera/list", HandleCameraList);
}
```

只有精确匹配的路由才注册到 routes。带 deviceId 的路由在 `Analysis::HandleRequest` 第二步通过 `GetDeviceId` 动态分发。

### 4.3 Dispatch — 路由匹配

```cpp
bool Dispatch(const std::string& path, CTL::Request& request,
              std::string& result, int& status) {
    auto it = routes.find(path);        // std::map 的 O(log n) 查找
    if (it != routes.end()) {
        result = it->second(request, status);  // 调用处理函数
        return true;
    }
    return false;                        // 未找到
}
```

---

## 五、路由处理函数详解

### 5.1 HandleRegister — 登录认证

```
路径:    POST /api/login
Body:    {"Account":"123456","Password":"123456"}
返回:    {"Account":"123456","Password":"123456","status":"success"}
         {"Account":"123456","Password":"123456","status":"fail"}
         400 Body 为空
         500 异常

逻辑:
  1. 检查 Body 是否为空 → 空则 400
  2. json.get("Account") 对比 user_message["Account"]（硬编码 "123456"）
  3. json.get("Password") 对比 user_message["Password"]（硬编码 "123456"）
  4. 两者都匹配 → status="success"，否则 "fail"
  5. 异常 → 500
```

### 5.2 HandleID — 分配摄像头 ID

```
路径:    GET /api/GetID
返回:    {"cam0":0}     // 分配了 ID 0
         {"cam1":1}     // 分配了 ID 1

逻辑:
  1. GetID() → 从 ID_t[20] 中找到第一个 false 的槽位，设为 true，返回索引
     （mutex 保护的 20 槽位 bool 数组，详见第六章）
  2. lock → all_id.push_back(i)   // 记录已分配的 ID
  3. 返回 {"camX": X}
```

关键：这个路由给摄像头终端（demo1）调用。终端启动时请求 `/api/GetID`，拿到自己的编号。

### 5.3 HandleCameraList — 在线摄像头列表

```
路径:    GET /api/camera/list
返回:    {"devices":[3,5,7],"count":3}

逻辑:
  CameraManager::Instance().OnlineDevices()
    → shared_lock sessionsMutex_
    → 遍历 sessions_ map
    → 每个 session->IsOnline()
        → IsOnline(): 当前时间 - lastHeartbeat_ < 15 秒 → true
    → 收集所有在线的 deviceId
    → 组装 JSON 返回
```

**注意：** 这里返回的是真正有 TCP 连接且心跳正常的在线设备，不是 ID.cpp 里那个静态 20 槽位数组。

### 5.4 HandleCameraFrame — 获取实时帧

```
路径:    GET /api/camera/3/frame
返回:    JPEG 二进制数据（注意不是 JSON！）

逻辑:
  1. 从 path 中解析 deviceId:
       path.substr(12) → "3/frame"
       找到 "/frame" → 截取 "3" → stoi → 3
  2. CameraManager::Instance().GetFrame(3)
       → GetSession(3) → 找到 CameraSession
       → session->GetLatestFrame()
           → lock(frameMutex_) → 返回 latestFrame_ 的拷贝
  3. 返回 std::string(frame.begin(), frame.end())

设计要点:
  - 返回的是 JPEG 字节，Content-Type 应该是 image/jpeg（由 SetWebServletFun 的 res.Write 处理）
  - latestFrame_ 由 CameraSession 的 recvThread 不断更新
  - 锁只保护读数据的过程，拿完立即释放，不阻塞 recvThread 写
```

### 5.5 HandleTakePhoto — 拍照

```
路径:    POST /api/camera/3/photo
返回:    {"photoId":"3_1717590100_a1b2.jpg","deviceId":3,"size":18432}

逻辑:
  1. 解析 deviceId（同上）
  2. CameraManager::Instance().TakePhoto(3)
       → GetSession(3)
       → session->TakePhoto(5000)
           ├─ SendCommand("TAKE_PHOTO\n")  → TCP 发给摄像头终端
           └─ wait_for(5秒, photoReady_==true)
               │
               │  ← 同时，CameraSession::recvThread 收到 TCP 回复:
               │      "PHOTO_DATA 18432\n" + [18432 字节 JPEG]
               │      ReadPhotoData(18432):
               │        lock(photoMutex_) → photoData_ = JPEG
               │        photoReady_ = true
               │        photoCv_.notify_one() → 唤醒 wait_for
               │
               └─ 被唤醒 → 返回 photoData_

  3. PhotoManager::Instance().SavePhoto(3, photo)
       生成文件名 "3_1717590100_a1b2.jpg"
       写文件 photos/3_1717590100_a1b2.jpg
       返回文件名作为 photoId

  4. 组装 JSON {"photoId":"3_1717590100_a1b2.jpg",...} 返回
```

**同步机制详解：**

`HandleTakePhoto` 在 HTTP 线程中执行。它调用 `session->TakePhoto()`，这个函数发出 TCP 指令后**阻塞等待**照片到达。照片由另一个线程（CameraSession 的 recvThread）收到后通过 `condition_variable` 唤醒等待者。

```
时间线:

HTTP 线程 (HandleTakePhoto)                    TCP 线程 (RecvLoop)
     │                                              │
     ├─ SendCommand("TAKE_PHOTO") ──TCP──→          │
     │                                              ├─ ProcessLine("PHOTO_DATA 18432")
     ├─ wait_for(5秒, photoReady_)                  ├─ ReadPhotoData(18432)
     │       阻塞中...                               │   ├─ socket->read → JPEG
     │                                              │   ├─ lock(photoMutex_)
     │                                              │   ├─ photoData_ = JPEG
     │                                              │   ├─ photoReady_ = true
     │                                              │   └─ unlock
     │        ← photoCv_.notify_one() ──────────────┤
     ├─ 被唤醒 → 返回 photoData_                     │
     │                                              │
     └─ PhotoManager::SavePhoto → 返回 photoId      │
```

### 5.6 HandleStreamControl — 推流控制

```
路径:    POST /api/camera/3/stream/start
        POST /api/camera/3/stream/stop
返回:    {"code":200}

逻辑:
  1. 从 path 中判断是 start 还是 stop
  2. 解析 deviceId
  3. start → CameraManager::Instance().StartStream(deviceId)
            → GetSession(deviceId)
            → session->SendCommand("STREAM_START\n")
     stop  → CameraManager::Instance().StopStream(deviceId)
            → session->SendCommand("STREAM_STOP\n")
```

**注意：** 推流启动后，摄像头终端开始编码帧并通过 TCP 持续发送 `STREAM_DATA`。服务端不主动轮询，而是被动收数据缓存。客户端需要自己定时调用 `GET /api/camera/{id}/frame` 来获取最新帧。

---

## 六、ID.cpp — ID 分配模块

### 6.1 数据结构

```cpp
inline static CCVector<bool> ID_t(20, false);  // 20 个槽位，false=未使用
std::mutex mutex_t;                             // 保护 ID_t
```

### 6.2 函数

```
GetID()
  lock → 遍历 ID_t[0..19]，找第一个 false → 设为 true → 返回索引
  全部 true → 返回 -1（无可用 ID）

ReleaseID(id)
  lock → ID_t[id] = false → 释放

LookAllID()
  lock → 返回 ID_t 的拷贝 → 调用者自己看哪些槽位被占用
```

**设计要点：** ID 分配是静态的 20 槽位，跟 CameraManager 里的 TCP 会话完全独立。`GetID` 只负责"发号"，不关心拿到号的摄像头是否真的在线。真正的在线状态由 `CameraSession::IsOnline()` 判断。

---

## 七、CameraSession — 与一个摄像头终端的 TCP 会话

### 7.1 类定义 (include/CameraSession.h)

```cpp
class CameraSession {
public:
    CameraSession(CTL::ClientSocket* sock);  // 接管 socket 所有权
    ~CameraSession();                         // close socket + join 线程 + delete

    // 禁止拷贝
    CameraSession(const CameraSession&) = delete;

    // 查询
    int  DeviceId() const;                   // 返回 deviceId_（-1 = 未注册）
    bool IsOnline() const;                   // lastHeartbeat_ < 15 秒?

    // 指令
    void SendCommand(const std::string& cmd); // 写一行到 TCP
    std::vector<char> GetLatestFrame() const; // 读缓存帧（HTTP 线程用）
    std::vector<char> TakePhoto(int timeoutMs = 5000); // 同步拍照（HTTP 线程用）

private:
    void RecvLoop();                          // 收 TCP 数据的线程
    void ProcessLine(const std::string& line);// 协议解析
    void ReadStreamData(size_t size);         // 读推流帧
    void ReadPhotoData(size_t size);          // 读照片数据

    CTL::ClientSocket* socket_;
    std::thread recvThread_;
    std::atomic<bool> running_{true};
    int deviceId_ = -1;
    std::chrono::steady_clock::time_point lastHeartbeat_;

    mutable std::mutex frameMutex_;           // 保护 latestFrame_
    std::vector<char> latestFrame_;           // JPEG 帧缓存

    std::mutex photoMutex_;                   // 保护 photoData_ + photoReady_
    std::condition_variable photoCv_;
    std::vector<char> photoData_;
    bool photoReady_ = false;
};
```

### 7.2 TCP 协议

```
摄像头终端 → 服务端:
  REGISTER 3\n                    注册（告诉服务端我是 3 号）
  STREAM_DATA 18432\n[18432字节]  推流帧
  PHOTO_DATA 9216\n[9216字节]     照片回传
  PONG\n                          保活回复

服务端 → 摄像头终端:
  STREAM_START\n                  开始推流
  STREAM_STOP\n                   停止推流
  TAKE_PHOTO\n                    拍照
  PING\n                          保活
```

协议设计：文本行（以 `\n` 结束）+ 可选二进制体。`STREAM_DATA` 和 `PHOTO_DATA` 后紧跟二进制 JPEG 数据，长度由文本行中的数字指定。

### 7.3 RecvLoop + ProcessLine — 接收循环

```
RecvLoop():
  while(running_):
    channel.readHeader()           // 从 socket 读一行文本
    → ProcessLine(line)

ProcessLine(line):
  如果 line 以 "REGISTER " 开头:
    deviceId_ = stoi(line.substr(9))  // "REGISTER 3" → 取 "3" → 3
  如果 line 以 "STREAM_DATA " 开头:
    size = stoull(line.substr(12))    // "STREAM_DATA 18432" → 18432
    → ReadStreamData(18432)           // 读 18432 字节存到 latestFrame_
  如果 line 以 "PHOTO_DATA " 开头:
    size = stoull(line.substr(11))    // "PHOTO_DATA 9216" → 9216
    → ReadPhotoData(9216)             // 读 9216 字节存到 photoData_ → notify
  如果 line == "PONG":
    lastHeartbeat_ = now()            // 刷新心跳时间
```

### 7.4 ReadStreamData — 缓存推流帧

```
ReadStreamData(jpegSize):
  socket->getInputStream()->read(data, jpegSize)
  如果读完整:
    lock(frameMutex_)
    latestFrame_ = vector<char>(data.buffer(), data.buffer() + data.size())
    unlock
```

**线程安全：** `latestFrame_` 被 recvThread 写、HTTP 线程读。`frameMutex_` 保护写入和读取，`GetLatestFrame` 返回的是拷贝，锁释放后数据仍然有效。

### 7.5 ReadPhotoData — 接收照片 + 通知等待者

```
ReadPhotoData(jpegSize):
  socket->getInputStream()->read(data, jpegSize)
  如果读完整:
    lock(photoMutex_)
    photoData_ = vector<char>(data.buffer(), data.buffer() + data.size())
    photoReady_ = true
    unlock
  photoCv_.notify_one()     ← 唤醒 TakePhoto 中阻塞的 HTTP 线程
```

**notify_one 放在 unlock 之后：** 避免唤醒的线程立刻又因为锁未释放而阻塞。

### 7.6 TakePhoto — 同步拍照

```
TakePhoto(timeoutMs):
  SendCommand("TAKE_PHOTO\n")            // TCP 发送指令
  lock(photoMutex_)
  wait_for(timeoutMs, photoReady_==true) // 阻塞等待通知
  如果超时 → 返回空
  如果收到通知:
    photoReady_ = false
    返回 photoData_
```

**时序图见 5.5 节。**

### 7.7 IsOnline — 在线判断

```
IsOnline():
  now - lastHeartbeat_ < 15秒 → true
  否则 → false
```

`lastHeartbeat_` 在构造时初始化为 now，在 `ProcessLine` 收到 `PONG` 时刷新。

### 7.8 析构顺序

```
~CameraSession():
  running_ = false             // 通知 RecvLoop 退出
  socket_->close()             // 关闭 socket，让阻塞的 read 返回
  recvThread_.join()           // 等待线程结束
  delete socket_               // 释放 socket
  socket_ = nullptr
```

**必须先 close 再 join：** `RecvLoop` 里 `readHeader()` 是阻塞的。如果不先 close socket，线程可能永远卡在 read 上，`join()` 永远不返回——死锁。

---

## 八、CameraManager — 摄像头会话管理器（单例）

### 8.1 类定义 (include/CameraManager.h)

```cpp
class CameraManager {
public:
    static CameraManager& Instance();      // 单例

    void Start(int port = 8083);           // 启动 TCP 服务器
    void Stop();

    // 查询（shared_lock，允许多个读者并发）
    std::vector<int> OnlineDevices() const;
    CameraSession*   GetSession(int deviceId) const;

    // 指令（转发到 CameraSession）
    std::vector<char> GetFrame(int deviceId) const;
    std::vector<char> TakePhoto(int deviceId) const;
    void StartStream(int deviceId) const;
    void StopStream(int deviceId) const;

private:
    CameraManager() = default;             // 私有 → 单例
    ~CameraManager();
    CameraManager(const CameraManager&) = delete;

    void AcceptLoop();                     // accept 线程

    CTL::ServerSocket* serverSocket_ = nullptr;
    std::thread acceptThread_;
    std::atomic<bool> running_{true};

    mutable std::shared_mutex sessionsMutex_;
    std::unordered_map<int, std::unique_ptr<CameraSession>> sessions_;
    //                    ↑ 独占所有权，出 map 自动 delete
};
```

### 8.2 单例

```cpp
CameraManager& CameraManager::Instance() {
    static CameraManager instance;   // C++11 保证线程安全的懒加载
    return instance;
}
```

第一次调用 `Instance()` 时创建，程序结束时自动析构。

### 8.3 Start — 启动 TCP 服务器

```cpp
Start(port):
  serverSocket_ = new CTL::ServerSocket(port)  // SDK 封装的 TCP Server
  acceptThread_ = thread(AcceptLoop)
```

### 8.4 AcceptLoop — accept 循环

```
AcceptLoop():
  while(running_):
    sock = serverSocket_->accept()         // 阻塞等新连接
    如果 sock 为空 → continue

    session = make_unique<CameraSession>(sock)  // 创建会话，启 recvThread

    // 等 5 秒让终端发 REGISTER
    deadline = now + 5秒
    while(now < deadline):
      如果 session->DeviceId() >= 0 → break
      sleep(100ms)

    如果超时还没注册 → continue（session 出作用域，自动 delete）

    id = session->DeviceId()

    lock(sessionsMutex_)                   // unique_lock（写锁）
    sessions_[id] = move(session)          // 放入 map
    unlock
```

**设计要点：**
- `unique_ptr` 确保 session 出作用域自动释放，不存在内存泄漏
- 写入用 `unique_lock`（独占），读取用 `shared_lock`（共享），允许多个 HTTP 请求同时查在线列表
- 5 秒等待是同步的——这段时间不能 accept 新连接。后续可优化为异步注册。

### 8.5 OnlineDevices — 在线设备列表

```cpp
OnlineDevices():
  shared_lock(sessionsMutex_)              // 共享锁，并发读
  for (auto& [id, session] : sessions_):
    如果 session->IsOnline() → result.push_back(id)
  return result
```

### 8.6 GetSession — 查找会话

```cpp
GetSession(deviceId):
  shared_lock(sessionsMutex_)
  it = sessions_.find(deviceId)
  如果找到 → return it->second.get()    // unique_ptr::get() 返回裸指针
  否则 → return nullptr

注意：返回的裸指针只在本次 HTTP 请求处理期间有效。
不要跨请求持有，因为会话可能随时断开被删除。
```

### 8.7 指令转发

```cpp
GetFrame(id):    GetSession(id) → session->GetLatestFrame()
TakePhoto(id):   GetSession(id) → session->TakePhoto()
StartStream(id): GetSession(id) → session->SendCommand("STREAM_START")
StopStream(id):  GetSession(id) → session->SendCommand("STREAM_STOP")
```

### 8.8 Stop — 关闭

```cpp
Stop():
  running_ = false
  delete serverSocket_           // 关闭监听 socket
  acceptThread_.join()
  // sessions_ 里的 unique_ptr 自动析构，每个 CameraSession 的析构函数会 close socket + join 线程
```

---

## 九、HeadBeat — UDP 心跳模块

### 9.1 类定义 (include/HeadBeat.h)

```cpp
struct HeartbeatPacket {
    char type[32];           // "HEARTBEAT" 或 "HEARTBEAT_ACK"
    uint32_t deviceId;       // 设备 ID
    uint64_t timestamp;      // 毫秒时间戳
};

class Headbeat {
public:
    Headbeat();
    ~Headbeat();                         // 调用 Stop()
    void Start(int deviceId = 0);
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

### 9.2 线程模型

```
Start():
  创建 socket_ (UDP, 广播)
  isRunning = true
  启 sendThread → SendLoop()
  启 recvThread → RecvLoop()

sendThread:                          recvThread:
  每轮 ~8 秒:                          持续阻塞:
    SendHeartbeat()                      socket_->receive() → 收到包
    等 3 秒收回复                        → OnHeartbeatReceived()
    休息 5 秒                               ├─ 校验 "HEARTBEAT"
                                           ├─ 打印 RTT
                                           └─ 回复 HEARTBEAT_ACK
```

### 9.3 SendHeartbeat

```cpp
SendHeartbeat():
  构造 HeartbeatPacket:
    type = "HEARTBEAT"
    deviceId = deviceId_++        // atomic 自增
    timestamp = nowMs()
  封装为 ByteArray
  socket_->broadcast(udpPacket)    // UDP 广播到 8082
```

**注意：** socket 在 `Start()` 中创建一次，`SendHeartbeat()` 只使用不分配。之前的版本每次 `SendHeartbeat` 都 `new` 一个新 socket 导致内存泄漏。

### 9.4 OnHeartbeatReceived — 处理外来心跳

```cpp
OnHeartbeatReceived(packet):
  校验 packet.type == "HEARTBEAT" → 否则忽略
  打印 "Heartbeat from device {id}"
  计算 RTT = nowMs() - packet.timestamp
  打印 RTT
  如果 RTT > 5000 → 打印 WARN

  // UDP 打洞：回复 ACK
  构造 HeartbeatPacket:
    type = "HEARTBEAT_ACK"
    deviceId = deviceId_
    timestamp = nowMs()
  socket_->broadcast(ackPkt)       // 回复到同一端口
```

**UDP 打洞原理：** 摄像头终端发 UDP 到服务端时，NAT 设备记录了"终端 IP:Port → 服务端 IP:Port"的映射。服务端回一个 UDP 包到同一地址，NAT 设备将包转发给终端——双向通路就此打通。

### 9.5 析构

```cpp
~Headbeat():
  Stop()              // RAII: 析构时自动释放资源
    isRunning = false
    join sendThread
    join recvThread
    delete socket_
    socket_ = nullptr
```

---

## 十、PhotoManager — 照片存储

### 10.1 类定义 (include/PhotoManager.h)

```cpp
class PhotoManager {
public:
    static PhotoManager& Instance();

    std::string SavePhoto(int deviceId, const std::vector<char>& jpeg);
    std::vector<char> GetPhoto(const std::string& photoId);
    std::vector<std::string> ListPhotos(int deviceId) const;

private:
    PhotoManager();                              // 创建 photos/ 目录
    std::string photosDir_ = "./photos";
    mutable std::shared_mutex mutex_;
};
```

### 10.2 构造 — 创建目录

```cpp
PhotoManager():
  #ifdef _WIN32
    _mkdir("./photos")         // Windows: <direct.h>
  #else
    mkdir("./photos", 0755)    // POSIX: <sys/stat.h>
  #endif
```

### 10.3 SavePhoto — 存照片

```
SavePhoto(deviceId, jpeg):
  生成文件名: "{deviceId}_{毫秒时间戳}_{4位随机十六进制}.jpg"
    → 例: "3_1717590100_a1b2.jpg"

  unique_lock(mutex_)           // 写锁，独占
  打开文件 photos/3_1717590100_a1b2.jpg
  写入 jpeg 二进制
  关闭文件

  返回文件名作为 photoId
```

**文件名唯一性：** 毫秒时间戳 + 4 位随机十六进制（65536 种可能），同一毫秒内碰撞概率极低。

### 10.4 GetPhoto — 取照片

```
GetPhoto(photoId):
  shared_lock(mutex_)           // 读锁，允许多个读者并发

  以二进制模式打开 photos/{photoId}
  用 std::ios::ate 打开 → 文件指针在末尾 → tellg() 获取大小
  seekg(0) → 回到开头
  read 全部字节 → 返回
```

### 10.5 ListPhotos — 列清单

```
ListPhotos(deviceId):
  前缀 = "{deviceId}_"  → 例: "3_"

  Windows:
    FindFirstFile("photos/3_*.jpg") → 遍历 → 收集文件名

  Linux:
    opendir("photos/") → readdir → 过滤前缀为 "3_" 的文件 → 收集文件名
```

### 10.6 对应的 HTTP 路由（需要新增，当前代码中缺失）

PhotoManager 的方法需要 HTTP 路由来暴露给客户端。当前代码中 `SavePhoto` 已在 `HandleTakePhoto` 中调用，但 `GetPhoto` 和 `ListPhotos` 缺少对外的路由。

需要新增两条路由：

```
GET  /api/photo/{photoId}       → PhotoManager::Instance().GetPhoto(photoId)
                                    → 返回 JPEG 二进制

GET  /api/photo/list/{deviceId} → PhotoManager::Instance().ListPhotos(deviceId)
                                    → 返回 {"photos":["3_xxx.jpg","3_yyy.jpg"]}
```

---

## 十一、编译配置 (CMakeLists.txt)

```cmake
cmake_minimum_required(VERSION 4.2)
project(API)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(CC_SDK)        # SDK 作为子项目

add_executable(API
    src/main.cpp
    src/URL.cpp        include/URL.h
    src/ID.cpp         include/ID.h
    src/Analysis.cpp   include/Analysis.h
    src/HeadBeat.cpp   include/HeadBeat.h
    src/CameraSession.cpp  include/CameraSession.h
    src/CameraManager.cpp  include/CameraManager.h
    src/PhotoManager.cpp   include/PhotoManager.h
)

target_include_directories(API PUBLIC ${CC_API_INC})  # SDK 头文件路径
target_link_libraries(API PUBLIC CC_API)               # SDK 静态库
```

---

## 十二、已知问题清单

| 文件 | 问题 | 严重度 |
|------|------|--------|
| `src/CameraSession.cpp:66` | `cmd.back() == '/r'` 应该是 `'\r'`，`'/n'` 应该是 `'\n'`。`/r` 是一个多字节字符字面量，语义错误 | 中 |
| `src/CameraSession.cpp:57` | `while(true)` 没用 `running_` 控制，依赖 socket close 导致 read 失败退出 | 低 |
| `src/CameraManager.cpp:81` | `TakePhoto` 调 `it->TakePhoto()` 没传超时参数 | 低（有默认值 5000） |
| `src/CameraManager.cpp:95` | AcceptLoop 等 REGISTER 是同步阻塞 5 秒 | 低（功能正确，性能可优化） |
| `include/CameraManager.h:26` | `HasDevice` 声明了但 `src/CameraManager.cpp` 中没有实现 | 低（未使用） |
| `src/PhotoManager.cpp:26-27` | `filename = "_" + ms + "_" + rand + ".jpg"` 缺少 deviceId，文件名格式应为 `"{deviceId}_{ms}_{rand}.jpg"` | 高 |
| `src/PhotoManager.cpp:29` | `fullpath = photosDir_ + "/" + "/" + filename` 多了一个 `"/"` | 高 |
| `src/PhotoManager.cpp:35` | `std::cerr << ("...", fullpath)` 逗号表达式只打印了 `fullpath`，格式化字符串被丢弃 | 低 |
| `src/PhotoManager.cpp:108` | `#else` 分支用了 `_mkdir` 应该是 `mkdir` | 中 |
| `src/ID.cpp:33-39` | `CameraOnline` 循环第一次迭代就 return，只检查了 `ID_t[0]` | 中 |
| `src/Analysis.cpp:49` | `path == "/api/camera/list"` 这个判断是冗余的——因为它已经在 HandleGet 的 URL::Dispatch 中被精确匹配了，不会走到这里 | 低 |
| `include/HeadBeat.h:5` | `#include "CCTimer.h"` 不再需要（Timer 成员已删除） | 低 |

---

## 十三、数据流向终图

```
┌──────────────┐     HTTP(8082)     ┌──────────────┐     TCP(8083)     ┌──────────────┐
│   客户端      │ ◄──────────────► │   服务端 (API) │ ◄──────────────► │  摄像头终端   │
│  (http_pro)   │                   │               │                   │  (demo1)     │
│               │                   │               │                   │              │
│  登录         │── POST /login ──→│ HandleRegister │                   │              │
│  看列表       │── GET  /list ───→│ CameraManager  │                   │  申请 ID     │
│               │                   │   .Online()    │←── GET /GetID ──│              │
│               │                   │               │                   │              │
│  看画面       │── GET  /3/frame →│ CameraManager  │                   │  编码发送    │
│               │←── JPEG ────────│   .GetFrame()   │←─ STREAM_DATA ──│  OpenCV帧    │
│               │                   │               │                   │              │
│  推流控制     │── POST /stream ─→│ CameraManager  │── STREAM_START ─→│  开始/停止   │
│               │                   │   .StartStream │                   │  编码       │
│               │                   │               │                   │              │
│  拍照         │── POST /3/photo →│ CameraManager  │── TAKE_PHOTO ──→│  拍照        │
│               │←── {photoId} ───│   .TakePhoto    │←─ PHOTO_DATA ──│  JPEG回传    │
│               │                   │ ┌─PhotoManager │                   │              │
│               │                   │ │ .SavePhoto   │                   │              │
│               │                   │ └→ 存为文件     │                   │              │
│               │                   │               │                   │              │
│               │                   │  HeadBeat      │── UDP ACK ─────→│  心跳        │
│               │                   │               │←── UDP ─────────│  打洞        │
└──────────────┘                   └──────────────┘                   └──────────────┘
```
