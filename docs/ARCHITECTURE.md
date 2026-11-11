# API 服务端架构总览

> 本文档是 API 服务端（C++17 摄像头管理服务）架构的**总纲**：架构缘起、借鉴来源、设计思路与原因、五层架构全貌。
> 详细分层说明见 [API_ARCHITECTURE.md](./API_ARCHITECTURE.md)，重构历史与行为对比见 [REFACTOR_CHANGES.md](./REFACTOR_CHANGES.md)。

---

## 目录

1. [架构缘起](#一架构缘起)
2. [借鉴的企业级架构思想](#二借鉴的企业级架构思想)
3. [从问题到设计决策](#三从问题到设计决策)
4. [五层架构总览](#四五层架构总览)
5. [依赖规则](#五依赖规则)
6. [各层职责](#六各层职责)
7. [关键机制](#七关键机制)
8. [通信全景](#八通信全景)
9. [设计原则](#九设计原则)
10. [相关文档](#十相关文档)

---

## 一、架构缘起

### 1.1 分层架构的演进脉络

分层架构不是凭空发明，而是对"软件腐烂"的递进式回应：

```
第 0 代：面条代码（所有逻辑堆在 main / 单文件里）
   ↓ 问题：无法理解、无法修改、无法测试
第 1 代：三层架构（表现层 / 业务逻辑层 / 数据访问层）
   ↓ 问题：业务层 import 了 JDBC/ORM 等细节，换数据库要改业务代码
第 2 代：六边形 / 洋葱 / 整洁架构（2003 ~ 2012）
   核心诉求：把"业务"变成不依赖任何框架的纯逻辑，
            把"数据库、HTTP、消息队列"全部降级为可插拔的适配器
```

### 1.2 本项目的旧架构问题（重构动因）

重构前的代码处于"第 0.5 代"——分了类，但带着典型的架构债务：

| 问题 | 旧代码表现 | 后果 |
|------|-----------|------|
| 全局单例 | `CameraManager::Instance()`、`PhotoManager::Instance()` | 隐式全局状态，不可替换、不可测试 |
| 静态可变状态 | `ID.cpp` 的 `ID_t[20]` 静态槽位数组 | 生命周期不可控 |
| 业务与协议混杂 | `URL.cpp` 路由处理函数里写业务 + 直接调单例 | 加路由要改分发逻辑，牵一发动全身 |
| 散落硬编码 | `192.168.1.175`、`8082`、`"123456"` | 改配置必须重编译 |
| 手工路径解析 | `path.substr(12)`、`path.find("/", start+1)` | 脆弱，新增路由成本高 |
| 错误传递混乱 | `status` 出参 + 空字符串表示失败 | 调用方分不清"没数据"和"出错了" |

**核心隐患不是"代码丑"，而是"不可测试、不可替换、不可演进"。**

---

## 二、借鉴的企业级架构思想

本次重构没有照搬任何一家公司的代码库，而是借鉴业界成熟的**通用架构模式体系**，并按 C++ 项目实际情况裁剪：

### 2.1 三个理论源头

| 源头 | 提出者/年份 | 借鉴了什么 |
|------|------------|-----------|
| **六边形架构**（Ports & Adapters） | Alistair Cockburn，2005 | 业务核心居中，HTTP/TCP/存储都是平级的"适配器"，通过"端口"接口与核心对话 |
| **整洁架构**（Clean Architecture） | Robert C. Martin，2012 | 依赖只能从外指向内；一切架构形态的本质是同一条依赖规则 |
| **领域驱动设计**（DDD） | Eric Evans，2003 | 领域概念（协议、模型）独立成层，防止基础设施代码污染业务模型 |

### 2.2 企业落地形态参照

| 参照 | 借鉴了什么 |
|------|-----------|
| Android 官方 App Architecture | UI → Domain → Data 三段划分；接口在 domain、实现在 data |
| Go 社区 go-kit | transport（接口）/ endpoint（用例）/ service 三层严格隔离 |
| Spring Boot 分层 + 构造器注入 | Controller → Service → Repository 职责划分；注入接口而非实现 |
| Netflix / Google 内部微服务 | 组合根（Composition Root）集中装配对象图 |

---

## 三、从问题到设计决策

设计顺序是"先诊断旧代码 → 每个问题对应一个设计决策"，而非先定五层再硬套：

| # | 旧代码问题 | 设计决策 | 落地产物 |
|---|-----------|---------|---------|
| 1 | 单例、全局状态 | 依赖注入 + 组合根 | `AppContext` 集中装配，`main.cpp` 仅 26 行 |
| 2 | 业务直接调用具体类 | 端口接口（依赖倒置） | `app/port/` 下 4 个抽象接口 |
| 3 | 协议字符串散落各处 | 独立领域层 | `domain/Protocol.h`、`PhotoMeta.h` |
| 4 | 出参 status + 空串表错 | 错误即数据 | `Result<T>` + `ErrorCode` 枚举 |
| 5 | 依赖方向靠自觉 | 编译期强制分层 | CMake 静态库拓扑 = 架构拓扑 |

### 为什么恰好是五层（而非三层或七层）

- **common 单独拆**：`Result`/`Logger`/`Config` 是被所有层引用的横切件，放任何业务层都会被"反向依赖"，独立最干净；
- **domain 单独拆**：协议常量是项目自己的"业务语言"，零依赖、最稳定，未来终端侧 SDK 可直接复用；
- **web 与 infra 平级**：六边形架构的关键特征——HTTP 不是更高级的存在，它和 TCP、UDP、WS 一样只是外部适配器；
- **AppContext 不在五层之内**：它是组合根，是"故意打破依赖规则的唯一例外"。

---

## 四、五层架构总览

```
┌──────────────────────────────────────────────────────────────┐
│                     组合根 AppContext                         │
│     唯一知道所有具体类的地方：装配、注入、生命周期管理            │
├──────────────────────────────────────────────────────────────┤
│  ④ web 接口层（HTTP 适配器）                                   │
│     Router / BaseController / HttpServer                      │
│     CameraController / PhotoController /                      │
│     AuthController / IdController                             │
├──────────────────────────────────────────────────────────────┤
│  ③ application 应用层（业务核心）                              │
│     端口(port)：ICameraChannel / ICameraChannelManager /      │
│                IPhotoRepository / IIdAllocator                │
│     用例(service)：CameraService / PhotoService / AuthService │
├──────────────────────────────────────────────────────────────┤
│  ⑤ infrastructure 基础设施层（TCP/UDP/WS/文件适配器）          │
│     TcpCameraChannel / CameraChannelManager /                 │
│     UdpHeartbeatService / WebSocketStreamService /            │
│     FilePhotoRepository / SlotIdAllocator                     │
├──────────────────────────────────────────────────────────────┤
│  ② domain 领域层     Protocol.h / PhotoMeta.h（纯头文件）      │
├──────────────────────────────────────────────────────────────┤
│  ① common 公共层     Result<T> / ErrorCode / Logger / Config  │
└──────────────────────────────────────────────────────────────┘
```

### 目录结构

```
API/
├── CMakeLists.txt                  # 分层静态库构建
├── config.json                     # 运行配置
├── include/api/
│   ├── AppContext.h                # 组合根
│   ├── common/                     # ① 公共层
│   ├── domain/                     # ② 领域层
│   ├── app/                        # ③ 应用层（port + service）
│   ├── web/                        # ④ 接口层（http + controller）
│   └── infra/                      # ⑤ 基础设施层（net/persistence/id）
├── src/
│   ├── main.cpp                    # 极简入口
│   └── api/AppContext.cpp          # 组合根实现
└── CC_SDK/                         # 第三方 SDK
```

---

## 五、依赖规则

```
web ──────────────┐
                  ▼
infrastructure ──▶ application ──▶ domain ──▶ common
                  （端口接口）     （纯值）    （基础件）
```

1. **依赖只指向下方**：上层可以 include 下层，反向禁止；
2. **web 与 infra 互不依赖**：接口层不知道通道是 TCP 实现的，基础设施层不知道 HTTP 存在；
3. **application 只依赖端口接口**：`CameraService` 拿到的只是 `ICameraChannelManager&`，与具体实现零耦合，可 mock、可替换；
4. **组合根是唯一例外**：AppContext 认识所有层，负责 new 出具体类并注入到各依赖者的端口上。

### CMake 编译期强制

```cmake
api_common        STATIC     # Result/ErrorCode/Logger/Config
api_domain        INTERFACE  # 纯头文件
api_application   STATIC     # 端口 + 用例
api_web           STATIC     # 路由 + 控制器
api_infrastructure STATIC    # 具体实现
API               EXECUTABLE # main + AppContext
```

- `target_link_libraries` 拓扑 = 架构拓扑：任何跨层反向依赖在**链接阶段直接报错**；
- `api_web` 与 `api_infrastructure` 之间没有链接关系，共同依赖 `api_application` 的端口；
- OpenCV 只被可执行文件链接，各层不直接依赖 OpenCV。

---

## 六、各层职责

### ① common 公共层 — 基础件

- **Result\<T\>**：统一结果类型，业务层不抛异常穿越层边界；`Result<void>` 特化支持无返回值操作；
- **ErrorCode**：统一业务错误码（kOk / kInvalidArgument / kUnauthorized / kNotFound / kDeviceOffline / kTimeout / kIoError / kNoIdAvailable / kInternal）；
- **Logger**：线程安全日志（LOG_INFO / LOG_WARN / LOG_ERROR / LOG_DEBUG）；
- **ServerConfig**：运行配置，从 config.json 加载，失败回退默认值。

### ② domain 领域层 — 协议与模型（纯头文件，零依赖）

- **Protocol.h**：TCP 指令协议常量

```
摄像头终端 → 服务端:  PONG / REQUEST_ID / LOGIN / STREAM_DATA / PHOTO_DATA
服务端 → 摄像头终端:  ASSIGN_ID / LOGIN_OK / LOGIN_FAIL / STREAM_START /
                      STREAM_STOP / TAKE_PHOTO
```

- **PhotoMeta.h**：照片元数据值对象 `{ photoId, deviceId, size }`。

### ③ application 应用层 — 端口 + 用例

**端口**（业务对外的能力需求声明，由 infra 实现）：

```cpp
ICameraChannel          // 一条摄像头 TCP 通道：DeviceId / IsOnline /
                        //   SendCommand / GetLatestFrame / TakePhoto
ICameraChannelManager   // 通道池：Start / Stop / OnlineDevices / Find
IPhotoRepository        // 照片仓库：Save / Load / List
IIdAllocator            // ID 分配：Allocate
```

**用例服务**（编排端口，不含任何 SDK/协议细节）：

| 服务 | 职责 | 依赖端口 |
|------|------|----------|
| CameraService | 在线列表 / 取帧 / 拍照 / 推流控制 | ICameraChannelManager, IPhotoRepository |
| PhotoService | 取照片 / 列清单 | IPhotoRepository |
| AuthService | 账号密码校验 | 无（值注入） |

### ④ web 接口层 — HTTP 适配器

- **Router**：`{id}` 占位符路由表，匹配结果提取到 `PathParams`；
- **BaseController**：统一响应封装（WriteJson / WriteError / WriteJpeg）+ ErrorCode→HTTP 映射；
- **HttpServer**：SDK `CTL::WebServlet` 薄封装；
- **4 个 Controller**：薄适配器——解析参数 → 调 service → 按 Result 写响应。

**HTTP 路由表（9 条）**：

| 方法 | 路径 | 控制器 |
|------|------|--------|
| POST | /api/login | AuthController |
| GET | /api/GetID | IdController |
| GET | /api/camera/list | CameraController |
| GET | /api/camera/{id}/frame | CameraController |
| POST | /api/camera/{id}/photo | CameraController |
| POST | /api/camera/{id}/stream/start | CameraController |
| POST | /api/camera/{id}/stream/stop | CameraController |
| GET | /api/photo/{photoId} | PhotoController |
| GET | /api/photo/list/{deviceId} | PhotoController |

### ⑤ infrastructure 基础设施层 — 具体实现

| 类 | 说明 |
|----|------|
| TcpCameraChannel | TCP 行/二进制双模式解析；帧缓存队列；拍照条件变量同步 |
| CameraChannelManager | accept 循环 + 注册等待（5s）+ shared_mutex 会话表 |
| UdpHeartbeatService | 广播心跳（发 → 等 3s 回复 → 休 5s）+ HEARTBEAT_ACK 打洞 |
| WebSocketStreamService | 独立端口向前端推送实时帧 |
| FilePhotoRepository | 照片文件存储，文件名 `_{ms}_{rand4}.jpg` |
| SlotIdAllocator | 实例化 20 槽位 ID 分配，池满返回 kNoIdAvailable |

---

## 七、关键机制

### 7.1 组合根装配（AppContext::Init）

```
1. infra：实例化 6 个基础设施类
2. app：  构造 3 个 Service，注入端口接口（shared_ptr 指向 infra 实现）
3. web：  构造 4 个 Controller，RegisterRoutes 注册 9 条路由
4. HTTP： httpServer_.Init(config, router_)
5. 启动： TCP 通道(8083) → UDP 心跳(8082 广播) → WS 推流(8081)
6. Run：  httpServer_.Run() 阻塞
7. 析构： 逆序停止 WS → UDP → TCP
```

### 7.2 错误传递：ErrorCode → HTTP 映射

| ErrorCode | HTTP | 场景 |
|-----------|------|------|
| kOk | 200 | 成功 |
| kInvalidArgument | 400 | 参数错误 |
| kUnauthorized | 401 | 未授权 |
| kNotFound / kDeviceOffline | 404 | 资源/设备不存在 |
| kNoIdAvailable | 503 | ID 池耗尽 |
| kTimeout | 504 | 拍照超时 |
| kIoError / kInternal | 500 | IO / 内部错误 |

### 7.3 拍照同步时序（跨线程协作）

```
HTTP 线程（CameraService::TakePhoto）       TCP 线程（TcpCameraChannel::RecvLoop）
  SendCommand("TAKE_PHOTO") ──TCP──→
  wait_for(5s, photoReady_)                 收到 PHOTO_DATA n → 读 n 字节
      阻塞中...                              photoReady_ = true
      ← notify_one ─────────────────────── photoCv_.notify_one()
  返回 Result<JPEG>（超时返回 kTimeout）
```

### 7.4 扩展新功能的标准路径

```
domain 定义概念 → app/port 声明接口 → app/service 写用例
→ infra 写实现 → web/controller 注册路由 → AppContext 一行装配
```

业务层与基础设施层互不修改，加功能不碰旧代码。

---

## 八、通信全景

```
客户端                HTTP 8082              服务端                  TCP 8083
http_pro ──────────── POST /api/login ───▶ AuthService
                     GET /api/camera/list ▶ CameraService ───▶ OnlineDevices()
                     GET /api/camera/{id}/frame ▶ 帧队列取帧 ◀─ STREAM_DATA
                     POST /api/camera/{id}/photo ▶ TakePhoto ──▶ TAKE_PHOTO
                                                              ◀─ PHOTO_DATA
                     POST .../stream/start|stop ──▶ SendCommand("STREAM_*")
                     GET /api/photo/{photoId} ▶ FilePhotoRepository::Load
                     GET /api/GetID ▶ SlotIdAllocator::Allocate

WebSocket 8081        WebSocketStreamService ──▶ 持续推送最新帧
UDP 8082 广播         UdpHeartbeatService ◀── HEARTBEAT ── 摄像头终端
                                         ──▶ HEARTBEAT_ACK（打洞）
```

| 通道 | 端口 | 用途 |
|------|------|------|
| HTTP | 8082 | 客户端与终端请求服务 |
| UDP | 8082 广播 | 终端心跳 + ACK 打洞 |
| TCP | 8083 | 终端长连接（注册/指令/推流帧/照片） |
| WebSocket | 8081 | 向前端持续推送实时帧 |

---

## 九、设计原则

1. **依赖倒置（DIP）**：业务层依赖端口抽象，不依赖具体实现；
2. **单一职责（SRP）**：协议解析、存储、路由、认证各归其类；
3. **显式依赖**：无全局单例、无静态状态，一切依赖经构造函数注入；
4. **错误即数据**：`Result<T>` 传递错误，不抛异常穿越层边界；
5. **编译期强制分层**：CMake 静态库依赖方向 = 架构依赖方向；
6. **组合根集中装配**：对象图只在 AppContext 一处构建，main.cpp 保持极简。

### 这套架构的价值

- **可测试**：注入 mock 端口即可单测业务逻辑，拍照超时/设备离线/ID 池耗尽无需真连硬件；
- **可替换**：CC_SDK 换通信库、文件存储换 MySQL，只改 infra 层，业务层连编译都不用变；
- **可演进**：加功能有标准路径，新同事看目录结构即懂代码位置；
- **编译期守门**：分层违规是 C++ 常见滑坡，静态库拓扑比 code review 更可靠。

---

## 十、相关文档

| 文档 | 内容 |
|------|------|
| [API_ARCHITECTURE.md](./API_ARCHITECTURE.md) | 五层架构详细说明（各层接口签名、协议处理、请求流程、配置详解） |
| [REFACTOR_CHANGES.md](./REFACTOR_CHANGES.md) | 重构改动说明（旧→新映射表、行为保持清单、验证结果） |

---

## 附：运行配置（config.json）

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

启动流程：加载 config.json（失败回退默认配置）→ AppContext 装配五层 → 依次启动 TCP/UDP/WS 服务 → HTTP 阻塞运行。
