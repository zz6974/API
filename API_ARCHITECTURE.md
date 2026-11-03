# API 服务端 — 企业级分层架构文档

> 本文档描述重构后的五层分层架构（六边形架构 / 端口-适配器模式）。
> 重构历史与行为对比见《REFACTOR_CHANGES.md》。

---

## 一、架构总览

### 1.1 这是什么

`API` 是一个 C++17 摄像头管理服务端，运行三个通道：

```
端口 8082 / HTTP   ← 客户端与摄像头终端通过 HTTP 请求服务（JSON + JPEG 二进制）
端口 8082 / UDP    ← 摄像头终端心跳（广播 + ACK 打洞）
端口 8083 / TCP    ← 摄像头终端长连接（注册、指令、推流帧、照片回传）
端口 8081 / WS     ← 向前端持续推送实时帧（WebSocket）
```

### 1.2 五层架构

```
┌────────────────────────────────────────────────────────────┐
│                    组合根 AppContext                        │
│   （唯一知道所有具体类的地方：装配、注入、生命周期管理）        │
├────────────────────────────────────────────────────────────┤
│  web 接口层     Router / BaseController / HttpServer        │
│                 CameraController / PhotoController /        │
│                 AuthController / IdController               │
├────────────────────────────────────────────────────────────┤
│  application 应用层                                          │
│   端口(port): ICameraChannel / ICameraChannelManager /      │
│              IPhotoRepository / IIdAllocator                │
│   用例(service): CameraService / PhotoService / AuthService │
├────────────────────────────────────────────────────────────┤
│  infrastructure 基础设施层                                   │
│   TcpCameraChannel / CameraChannelManager /                 │
│   UdpHeartbeatService / WebSocketStreamService /            │
│   FilePhotoRepository / SlotIdAllocator                     │
├────────────────────────────────────────────────────────────┤
│  domain 领域层     Protocol.h / PhotoMeta.h （纯头文件）      │
├────────────────────────────────────────────────────────────┤
│  common 公共层     Result<T> / ErrorCode / Logger / Config  │
└────────────────────────────────────────────────────────────┘
```

### 1.3 依赖规则

```
web ──────────────┐
                  ▼
infrastructure ──▶ application ──▶ domain ──▶ common
                  （端口接口）      （纯值）     （基础件）
```

- **依赖只指向下方**：上层可以 include 下层，反之禁止。由 CMake 静态库依赖在编译期强制。
- **web 与 infrastructure 互不依赖**：接口层不知道通道是 TCP 实现的，基础设施层不知道 HTTP 存在。
- **application 只依赖端口接口**：`CameraService` 拿到的只是 `ICameraChannelManager&`，与 `CameraChannelManager` 无任何耦合，可替换、可 mock。
- **组合根（AppContext）是唯一例外**：它同时认识所有层，负责 new 出具体类并注入到各自依赖者的端口上。

---

## 二、目录结构

```
API/
├── CMakeLists.txt                  # 分层静态库构建
├── config.json                     # 运行配置（替代硬编码）
├── include/api/
│   ├── AppContext.h                # 组合根声明
│   ├── common/                     # ① 公共层
│   │   ├── ErrorCode.h             # 统一错误码
│   │   ├── Result.h                # Result<T> 统一结果
│   │   ├── Logger.h                # 线程安全日志
│   │   └── Config.h                # ServerConfig 配置结构
│   ├── domain/                     # ② 领域层（纯头文件）
│   │   ├── Protocol.h              # TCP 协议常量
│   │   └── PhotoMeta.h             # 照片元数据值对象
│   ├── app/                        # ③ 应用层
│   │   ├── port/                   #    端口接口（抽象）
│   │   │   ├── ICameraChannel.h
│   │   │   ├── ICameraChannelManager.h
│   │   │   ├── IPhotoRepository.h
│   │   │   └── IIdAllocator.h
│   │   └── service/                #    用例服务（编排）
│   │       ├── CameraService.h/.cpp
│   │       ├── PhotoService.h/.cpp
│   │       └── AuthService.h/.cpp
│   ├── web/                        # ④ 接口层
│   │   ├── http/
│   │   │   ├── HttpTypes.h         # PathParams / Handler
│   │   │   ├── Router.h/.cpp       # {id} 占位符路由
│   │   │   ├── BaseController.h/.cpp
│   │   │   └── HttpServer.h/.cpp   # SDK WebServlet 封装
│   │   └── controller/
│   │       ├── CameraController.h/.cpp
│   │       ├── PhotoController.h/.cpp
│   │       ├── AuthController.h/.cpp
│   │       └── IdController.h/.cpp
│   └── infra/                      # ⑤ 基础设施层
│       ├── net/
│       │   ├── TcpCameraChannel.h/.cpp
│       │   ├── CameraChannelManager.h/.cpp
│       │   ├── UdpHeartbeatService.h/.cpp
│       │   └── WebSocketStreamService.h/.cpp
│       ├── persistence/
│       │   └── FilePhotoRepository.h/.cpp
│       └── id/
│           └── SlotIdAllocator.h/.cpp
├── src/
│   ├── main.cpp                    # 极简入口
│   └── api/
│       ├── AppContext.cpp          # 组合根实现
│       └── common/Config.cpp       # config.json 解析
├── CC_SDK/                         # 第三方 SDK（CMake 子项目）
└── photos/                         # 照片目录（运行时创建）
```

---

## 三、各层详解

### 3.1 common 公共层 — 基础件

**Result\<T\>**（Result.h）：统一结果类型，业务层不抛异常穿越层边界。

```cpp
Result<T>::Ok(value)                       // 成功携带值
Result<T>::Fail(ErrorCode, message)        // 失败携带错误码
Result<void> 特化                          // 无返回值操作
```

**ErrorCode**（ErrorCode.h）：统一业务错误码，由 web 层映射为 HTTP 状态码：

| ErrorCode | 含义 | HTTP 映射 |
|-----------|------|-----------|
| kOk | 成功 | 200 |
| kInvalidArgument | 参数错误 | 400 |
| kUnauthorized | 未授权 | 401 |
| kNotFound | 资源不存在 | 404 |
| kDeviceOffline | 设备不存在或离线 | 404 |
| kTimeout | 操作超时 | 504 |
| kIoError | IO 错误 | 500 |
| kNoIdAvailable | ID 池耗尽 | 503 |
| kInternal | 内部错误 | 500 |

**Logger**：线程安全日志宏 `LOG_INFO / LOG_WARN / LOG_ERROR / LOG_DEBUG`，支持级别过滤。

**ServerConfig**（Config.h）：运行配置，`LoadFromJson` 从 config.json 加载，失败可回退 `Default()`。

### 3.2 domain 领域层 — 协议与模型（纯头文件，零依赖）

**Protocol.h**：TCP 指令通道协议常量：

```
摄像头终端 → 服务端:  PONG / REQUEST_ID / LOGIN / STREAM_DATA / PHOTO_DATA
服务端 → 摄像头终端:  ASSIGN_ID / LOGIN_OK / LOGIN_FAIL / STREAM_START /
                      STREAM_STOP / TAKE_PHOTO
```

**PhotoMeta.h**：照片元数据值对象 `{ photoId, deviceId, size }`。

### 3.3 application 应用层 — 端口 + 用例

**端口接口**（app/port，抽象，由 infra 实现）：

```cpp
class ICameraChannel {                       // 一条摄像头 TCP 通道
    virtual int DeviceId() const = 0;
    virtual bool IsOnline() const = 0;
    virtual void SendCommand(const std::string&) = 0;
    virtual std::vector<char> GetLatestFrame() = 0;
    virtual Result<std::vector<char>> TakePhoto(int timeoutMs) = 0;
};

class ICameraChannelManager {                // 通道池
    virtual void Start(int port) = 0;
    virtual void Stop() = 0;
    virtual std::vector<int> OnlineDevices() const = 0;
    virtual std::shared_ptr<ICameraChannel> Find(int deviceId) = 0;
};

class IPhotoRepository {                     // 照片仓库
    virtual Result<std::string> Save(int deviceId, const std::vector<char>&) = 0;
    virtual Result<std::vector<char>> Load(const std::string& photoId) = 0;
    virtual Result<std::vector<std::string>> List(int deviceId) = 0;
};

class IIdAllocator {                         // ID 分配
    virtual Result<int> Allocate() = 0;
};
```

**用例服务**（app/service，只依赖端口）：

| 服务 | 职责 | 依赖端口 |
|------|------|----------|
| CameraService | 在线列表 / 取帧 / 拍照 / 推流控制（编排通道 + 照片仓库） | ICameraChannelManager, IPhotoRepository |
| PhotoService | 按 ID 取照片 / 按设备列清单 | IPhotoRepository |
| AuthService | 账号密码校验 | 无（值注入） |

### 3.4 web 接口层 — 路由与控制器

**Router**：支持 `{id}` 占位符的路由表。`Add("GET", "/api/camera/{id}/frame", handler)` 注册，
匹配时将占位符提取到 `PathParams`（`std::map<string,string>`）。替代旧的手工 substr 路径解析。

**BaseController**：控制器基类，提供统一响应封装：
- `WriteJson` — JSON 响应
- `WriteError` — 错误响应（`{"code":..,"msg":..}`）
- `WriteJpeg` — JPEG 二进制响应
- `ToHttpStatus` — ErrorCode → HTTP 状态码映射（见 3.1 表）
- `GetIntParam` — 路径参数转 int

**HttpServer**：SDK `CTL::WebServlet` 的薄封装，所有请求统一交给 Router 分发，未匹配返回 404。

**HTTP 路由表**（9 条，与重构前一致）：

| 方法 | 路径 | 控制器 | 返回 |
|------|------|--------|------|
| POST | /api/login | AuthController | `{"Account":..,"Password":..,"status":"success"}` |
| GET | /api/GetID | IdController | `{"cam0":0}` |
| GET | /api/camera/list | CameraController | `{"devices":[...],"count":n}` |
| GET | /api/camera/{id}/frame | CameraController | JPEG 二进制 |
| POST | /api/camera/{id}/photo | CameraController | `{"photoId":..,"deviceId":..,"size":..}` |
| POST | /api/camera/{id}/stream/start | CameraController | `{"code":200}` |
| POST | /api/camera/{id}/stream/stop | CameraController | `{"code":200}` |
| GET | /api/photo/{photoId} | PhotoController | JPEG 二进制 |
| GET | /api/photo/list/{deviceId} | PhotoController | `{"photos":[...]}` |

控制器是**薄适配器**：解析参数 → 调用 service → 用 Result 结果写响应，不含业务逻辑。

### 3.5 infrastructure 基础设施层 — 具体实现

| 类 | 说明 |
|----|------|
| TcpCameraChannel | 实现 ICameraChannel；TCP 收发行/二进制双模式解析；帧缓存 `CTL::Queue`；拍照条件变量同步 |
| CameraChannelManager | 实现 ICameraChannelManager；accept 循环 + 注册等待（5s）+ `shared_mutex` 保护的会话表 |
| UdpHeartbeatService | 广播心跳（每 ~8s 一轮：发 → 等 3s 回复 → 休 5s）+ `HEARTBEAT_ACK` 打洞 |
| WebSocketStreamService | 独立端口向前端推送摄像头最新帧（`SendBinary`） |
| FilePhotoRepository | 实现 IPhotoRepository；文件名 `_{ms}_{rand4}.jpg`；`shared_mutex` 读写并发 |
| SlotIdAllocator | 实现 IIdAllocator；实例化 20 槽位，池满返回 `kNoIdAvailable` |

**TcpCameraChannel 协议处理**（RecvLoop 逐字节解析）：

```
行模式：读到 \n 结束一行 → ProcessLine
  PONG          → 刷新 lastHeartbeat_
  REQUEST_ID    → allocator->Allocate() → 回 ASSIGN_ID{id}
  LOGIN         → 账号密码比对 → 回 LOGIN_OK / LOGIN_FAIL
  STREAM_DATA n → 进入二进制模式，读 n 字节 → StoreFrame（帧队列）
  PHOTO_DATA n  → 进入二进制模式，读 n 字节 → StorePhoto（通知等待者）
```

**拍照同步**（HTTP 线程与 TCP 线程协作）：

```
HTTP 线程（CameraService::TakePhoto）         TCP 线程（TcpCameraChannel::RecvLoop）
  SendCommand("TAKE_PHOTO") ──TCP──→
  wait_for(5s, photoReady_)                   收到 PHOTO_DATA n → 读 n 字节
      阻塞中...                                photoReady_ = true
      ← notify_one ───────────────────────── photoCv_.notify_one()
  返回 Result<JPEG>                            （超时返回 kTimeout）
```

### 3.6 组合根 AppContext

```
AppContext::Init(config)
  1. infra：实例化 SlotIdAllocator(20) / FilePhotoRepository(photoDir)
            / CameraChannelManager(allocator, account, password, registerTimeout)
            / UdpHeartbeatService / WebSocketStreamService(channelManager)
  2. app： 构造 3 个 Service，注入端口接口（shared_ptr 指向 infra 实现）
  3. web： 构造 4 个 Controller，RegisterRoutes(router_) 注册 9 条路由
  4. HTTP：httpServer_.Init(config, router_)
  5. 启动：channelManager->Start(tcpPort) / heartbeat->Start(addr, httpPort, 0)
          / webSocketStream->Start(wsAddr, wsPort)
  6. Run() → httpServer_.Run() 阻塞
  7. 析构/Shutdown() 按逆序停止 WS → UDP → TCP
```

**扩展新功能的标准路径**：domain 定义模型 → app/port 加接口（如需新存储/新通道）
→ app/service 写用例 → infra 写实现 → web/controller 注册路由 → AppContext 一行装配。
业务层与基础设施层都不需要改动彼此的代码。

---

## 四、请求处理流程（示例：GET /api/camera/3/frame）

```
HTTP 请求 ──▶ HttpServer(WebServlet 封装)
              │  全部请求
              ▼
             Router::Route(req, res)
              │  路径分段匹配 "/api/camera/{id}/frame" → params{"id":"3"}
              ▼
             CameraController::HandleFrame
              │  GetIntParam → 3
              ▼
             CameraService::GetFrame(3)
              │  channels_->Find(3)         ← 只认识 ICameraChannelManager 端口
              ▼
             CameraChannelManager::Find     ← infra 实现：shared_mutex 查会话表
              ▼
             TcpCameraChannel::GetLatestFrame  ← 帧队列取最新帧（线程安全）
              ▲
             Result<vector<char>> JPEG
              │
             BaseController::WriteJpeg  ──▶ HTTP 200 image/jpeg
```

---

## 五、配置（config.json）

```json
{
    "httpAddress": "192.168.1.175",
    "httpPort": 8082,
    "tcpPort": 8083,
    "wsAddress": "192.168.1.175",
    "wsPort": 8081,
    "photoDir": "./photos",
    "photoTimeoutMs": 5000,
    "registerTimeoutMs": 5000,
    "onlineTimeoutSeconds": 30,
    "account": "123456",
    "password": "123456"
}
```

| 字段 | 说明 |
|------|------|
| httpAddress / httpPort | HTTP 服务地址与端口 |
| tcpPort | 摄像头终端 TCP 指令通道端口 |
| wsAddress / wsPort | WebSocket 推流地址与端口 |
| photoDir | 照片存储目录 |
| photoTimeoutMs | 拍照等待超时（毫秒） |
| registerTimeoutMs | 新连接注册等待超时（毫秒） |
| onlineTimeoutSeconds | 心跳超时判定（秒），超过即视为离线 |
| account / password | 终端登录认证 |

`main.cpp` 加载失败时回退 `ServerConfig::Default()`（0.0.0.0:8082/8083/8081），保证程序不会因缺配置崩溃。

---

## 六、构建系统

分层静态库（CMakeLists.txt）：

```cmake
api_common        STATIC     # Result/ErrorCode/Logger/Config
api_domain        INTERFACE  # 纯头文件
api_application   STATIC     # 端口 + 用例（link api_domain）
api_web           STATIC     # 路由 + 控制器（link api_application）
api_infrastructure STATIC    # 具体实现（link api_application）
API               EXECUTABLE # main + AppContext（link api_web + api_infrastructure）
```

- 依赖方向由 `target_link_libraries` 强制：web 与 infrastructure 无法互相 include。
- `api_infrastructure` 与 `api_web` 之间没有链接关系，共同依赖 `api_application` 的端口。
- OpenCV 只被可执行文件链接，各层不直接依赖 OpenCV。

---

## 七、三端通信全貌

```
客户端                HTTP 8082              服务端                  TCP 8083
http_pro ──────────── POST /api/login ───▶ AuthService
                     GET /api/camera/list ▶ CameraService ───▶ OnlineDevices()
                     GET /api/camera/{id}/frame ▶ 帧队列取帧 ◀─ STREAM_DATA（推流帧）
                     POST /api/camera/{id}/photo ▶ TakePhoto ──▶ TAKE_PHOTO
                                                              ◀─ PHOTO_DATA（照片）
                     POST .../stream/start|stop ──▶ SendCommand("STREAM_START|STOP")
                     GET /api/photo/{photoId} ▶ FilePhotoRepository::Load
                     GET /api/GetID ▶ SlotIdAllocator::Allocate

WebSocket 8081        WebSocketStreamService ──▶ 持续推送最新帧
UDP 8082 广播         UdpHeartbeatService ◀── HEARTBEAT ── 摄像头终端
                                         ──▶ HEARTBEAT_ACK（打洞）
```

---

## 八、设计原则小结

1. **依赖倒置（DIP）**：业务层依赖端口抽象，不依赖具体实现；
2. **单一职责（SRP）**：协议解析、存储、路由、认证各归其类；
3. **显式依赖**：无全局单例、无静态状态，一切依赖经构造函数注入；
4. **错误即数据**：`Result<T>` 传递错误，不抛异常穿越层边界；
5. **编译期强制分层**：CMake 静态库依赖方向 = 架构依赖方向；
6. **组合根集中装配**：对象图只在 AppContext 一处构建，main.cpp 保持极简。
