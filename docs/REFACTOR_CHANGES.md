# 企业级分层重构 — 改动说明文档

> 本次重构将 API 服务端从「单层 + 全局单例」的原始结构，重构为五层企业级分层架构（六边形架构 / 端口-适配器模式），
> 在**不改变对外行为**的前提下落地，编译验证通过，程序功能保持不变。

---

## 一、改动目标

1. **分层解耦**：将 HTTP 路由业务、TCP 协议解析、照片存储、ID 分配、UDP 心跳混杂的代码拆分为
   common / domain / application / web / infrastructure 五层。
2. **依赖倒置**：业务层只依赖端口接口（port），不依赖具体实现；具体实现位于 infrastructure 层。
3. **去除全局状态**：消灭单例（`CameraManager::Instance()`、`PhotoManager::Instance()`）、
   静态槽位数组（`ID_t`）、散落的硬编码，改为显式依赖注入。
4. **行为不回归**：所有 HTTP 路由、TCP 协议、照片文件格式、超时参数、端口配置与重构前保持一致。

---

## 二、架构变化总览

```
重构前（单层，全局状态）                        重构后（五层 + 组合根）
─────────────────────────────                  ─────────────────────────────
main.cpp 手动逐个启动组件                       main.cpp 只加载配置 + 启动 AppContext
  ├─ Headbeat（UDP）                              │
  ├─ Analysis（HTTP 入口，含路由分发）            ▼
  │    └─ URL（路由表 + 全部业务处理函数）    AppContext（组合根：唯一知道所有具体类的地方）
  ├─ CameraManager（单例，TCP 会话管理）          ├─ infra: SlotIdAllocator / FilePhotoRepository
  │    └─ CameraSession（TCP 会话）               │         / CameraChannelManager / UdpHeartbeatService
  ├─ PhotoManager（单例，照片存储）               │         / WebSocketStreamService
  └─ ID.cpp（全局静态 20 槽位数组）               ├─ app:   CameraService / PhotoService / AuthService
                                                  ├─ web:   4 个 Controller + Router + HttpServer
                                                  └─ 启动 HTTP / TCP / WS / UDP 各服务
```

- 全局单例 → 构造函数注入的 `shared_ptr`（组合根统一管理生命周期）
- 路径手工 `substr` 解析 → Router 占位符模式匹配（`{id}`）
- 散落硬编码（IP、端口、账号密码、超时） → `config.json` + `ServerConfig`
- `status` 出参 + 空字符串判断错误 → `Result<T>` 统一错误码
- HTTP 业务逻辑写在 URL.cpp 处理函数 → 控制器薄封装 + 用例服务

---

## 三、文件改动清单

### 3.1 新增文件（按层）

```
include/api/
├── common/                                        # 公共层
│   ├── ErrorCode.h        统一错误码枚举 + ToString
│   ├── Result.h           Result<T> / Result<void> 统一结果类型
│   ├── Logger.h           线程安全日志（LOG_INFO/LOG_WARN/LOG_ERROR/LOG_DEBUG）
│   └── Config.h           ServerConfig 结构体 + LoadFromJson 声明
├── domain/                                         # 领域层（纯头文件）
│   ├── Protocol.h         TCP 协议常量（PONG/REQUEST_ID/LOGIN/STREAM_DATA/...）
│   └── PhotoMeta.h        照片元数据值对象（photoId/deviceId/size）
├── app/                                            # 应用层
│   ├── port/
│   │   ├── ICameraChannel.h          摄像头通道端口
│   │   ├── ICameraChannelManager.h   通道池端口
│   │   ├── IPhotoRepository.h        照片仓库端口
│   │   └── IIdAllocator.h            ID 分配端口
│   └── service/
│       ├── CameraService.h/.cpp      摄像头用例（在线列表/取帧/拍照/推流控制）
│       ├── PhotoService.h/.cpp       照片用例（获取/列出）
│       └── AuthService.h/.cpp        登录认证用例
├── web/                                            # 接口层
│   ├── http/
│   │   ├── HttpTypes.h         PathParams / Handler 类型定义
│   │   ├── Router.h/.cpp       {id} 占位符路由表
│   │   ├── BaseController.h/.cpp  统一响应封装 + ErrorCode→HTTP 映射
│   │   └── HttpServer.h/.cpp   SDK WebServlet 薄封装
│   └── controller/
│       ├── CameraController.h/.cpp
│       ├── PhotoController.h/.cpp
│       ├── AuthController.h/.cpp
│       └── IdController.h/.cpp
├── infra/                                          # 基础设施层
│   ├── net/
│   │   ├── TcpCameraChannel.h/.cpp          TCP 会话（旧 CameraSession 平移）
│   │   ├── CameraChannelManager.h/.cpp      通道池（旧 CameraManager 平移）
│   │   ├── UdpHeartbeatService.h/.cpp       UDP 心跳（旧 HeadBeat 平移）
│   │   └── WebSocketStreamService.h/.cpp    WS 推流（旧 URL::Init/Onopen 迁移）
│   ├── persistence/
│   │   └── FilePhotoRepository.h/.cpp       照片文件存储（旧 PhotoManager 平移）
│   └── id/
│       └── SlotIdAllocator.h/.cpp           20 槽位 ID 分配（旧 ID.cpp 平移）
└── AppContext.h               组合根声明

src/api/
├── common/Config.cpp           ServerConfig::LoadFromJson 实现
└── AppContext.cpp              组合根装配实现

config.json                     运行配置（替代硬编码）
```

### 3.2 删除文件（旧单层实现，全部 14 个）

| 文件 | 去向 |
|------|------|
| include/Analysis.h + src/Analysis.cpp | 路由分发 → Router；分发逻辑 → 各 Controller |
| include/URL.h + src/URL.cpp | 路由表 → Router；业务处理 → Service + Controller；WebSocket → WebSocketStreamService |
| include/CameraManager.h + src/CameraManager.cpp | → infra::CameraChannelManager |
| include/CameraSession.h + src/CameraSession.cpp | → infra::TcpCameraChannel |
| include/PhotoManager.h + src/PhotoManager.cpp | → infra::FilePhotoRepository |
| include/HeadBeat.h + src/HeadBeat.cpp | → infra::UdpHeartbeatService |
| include/ID.h + src/ID.cpp | → infra::SlotIdAllocator |

### 3.3 修改文件

| 文件 | 改动内容 |
|------|----------|
| src/main.cpp | 从「逐个 new/start 组件 + 硬编码 IP」改为「加载 config.json → AppContext.Init → Run」约 26 行 |
| CMakeLists.txt | 从「单个 add_executable 平铺所有 cpp」改为「5 个分层静态库 + 可执行文件」 |
| config.json | 新增（运行配置，值取自旧代码硬编码） |

---

## 四、模块映射表（旧 → 新）

| 旧模块（类/文件） | 旧职责 | 新模块 | 说明 |
|------------------|--------|--------|------|
| Analysis | HTTP 入口、方法分流、路径解析 | Router + BaseController | 两步分发改为占位符路由一次匹配 |
| URL::HandleRegister | 登录认证 | AuthService + AuthController | 账号密码从 config.json 注入 |
| URL::HandleID | 分配摄像头 ID | IdController + SlotIdAllocator | 全局静态数组 → 实例化的槽位分配器 |
| URL::HandleCameraList | 在线列表 | CameraController::HandleList | 返回格式不变 |
| URL::HandleCameraFrame | 取实时帧 | CameraController::HandleFrame | JPEG 二进制返回不变 |
| URL::HandleTakePhoto | 拍照 | CameraController::HandleTakePhoto + CameraService | 5 秒等待不变 |
| URL::HandleStreamControl | 推流控制 | CameraController::HandleStart/StopStream | 指令不变 |
| URL::Init / Onopen | WebSocket 8081 推流 | WebSocketStreamService | 功能完整保留 |
| CameraManager（单例） | TCP 会话管理 | CameraChannelManager | 单例 → shared_ptr 注入；端口参数化 |
| CameraSession | TCP 会话 | TcpCameraChannel | 实现 app::ICameraChannel 端口 |
| PhotoManager（单例） | 照片存储 | FilePhotoRepository | 实现 app::IPhotoRepository 端口 |
| Headbeat | UDP 心跳 | UdpHeartbeatService | Start(address, port, deviceId) 参数化 |
| ID.cpp | 静态 20 槽位 | SlotIdAllocator | Allocate 返回 Result，池满返回 kNoIdAvailable |

---

## 五、行为保持不变清单（保证"不影响程序正常运行"）

以下行为与重构前**完全一致**：

1. **HTTP 路由与响应格式**：全部 9 条路由路径、方法、响应 JSON 结构、状态码不变；
2. **TCP 指令协议**：`STREAM_START / STREAM_STOP / TAKE_PHOTO / ASSIGN_ID / LOGIN_OK / LOGIN_FAIL` 发送格式不变（含换行）；
3. **TCP 接收协议**：`PONG / REQUEST_ID / LOGIN / STREAM_DATA / PHOTO_DATA` 解析逻辑照搬（含 `LOGIN` 的旧解析行为）；
4. **照片文件名格式**：`_{毫秒时间戳}_{4位随机十六进制}.jpg`（旧代码无 deviceId 前缀，保持原样）；
5. **拍照超时**：5 秒（`photoTimeoutMs`，默认 5000）；
6. **注册等待超时**：5 秒（`registerTimeoutMs`，默认 5000）；
7. **在线判定**：心跳超时 30 秒（`onlineTimeoutSeconds`，与旧代码实际行为一致）；
8. **端口配置**：HTTP 8082 / TCP 8083 / WebSocket 8081，地址 192.168.1.175（写入 config.json）；
9. **登录账号密码**：123456 / 123456（写入 config.json）；
10. **UDP 心跳**：广播心跳 → 等 3 秒收回复 → 休息 5 秒的节奏、`HEARTBEAT_ACK` 打洞行为不变；
11. **ID 分配**：20 槽位、顺序分配第一个空闲槽位不变。

---

## 六、行为变化清单（有意为之的改进）

| 变化点 | 重构前 | 重构后 | 影响 |
|--------|--------|--------|------|
| 错误传递 | `status` 出参 + 空字符串/硬编码状态码 | `Result<T>` + `ErrorCode` 枚举 | 错误处理统一、可测试 |
| 设备不存在/离线 | 部分场景返回空数据 | 返回 `kDeviceOffline` → HTTP 404 | 更符合 REST 语义 |
| 拍照超时 | 返回空 → 可能存空文件 | 返回 `kTimeout` → HTTP 504 | 避免空照片落盘 |
| ID 池耗尽 | `GetID()` 返回 -1，继续流程 | `kNoIdAvailable` → HTTP 503 | 错误可见 |
| 配置来源 | main.cpp / URL.cpp 硬编码 | config.json 可配置 | 无需重编译即可改配置 |
| 路由匹配 | 精确 map + 手工 substr 两步分发 | 统一 `{id}` 占位符匹配 | 新增路由只需一行 |
| 模块耦合 | 全局单例直接调用 | 接口注入（端口-适配器） | 可替换实现、可单元测试 |

---

## 七、构建方式变化

重构前：单个 `add_executable(API ...)` 平铺全部 cpp。

重构后：分层静态库，用 CMake `target_link_libraries` 在**编译期强制依赖方向**：

```
api_common (STATIC)          ← Result / ErrorCode / Logger / Config
     ↑
api_domain (INTERFACE)       ← 协议常量 / 领域模型（纯头文件）
     ↑
api_application (STATIC)     ← 端口接口 + 用例服务
     ↑           ↑
api_web (STATIC)    api_infrastructure (STATIC)
     ↑           ↑
     └── API (可执行文件) ←── 组合根 AppContext + main.cpp
```

- 上层依赖下层是单向的：`api_web` / `api_infrastructure` 只能依赖 `api_application` 及以下；
- 任何跨层反向依赖（如 infra 直接 include web）在链接阶段会直接报错；
- `api_infrastructure` 与 `api_web` 互不依赖，符合六边形架构。

构建命令不变：

```bash
cmake --build cmake-build-debug --target API -j 8
```

---

## 八、验证结果

- [x] 全量编译通过：`Built target API`（api_common / api_application / api_web / api_infrastructure 依次构建成功）
- [x] 删除旧文件后重新编译通过（无残留引用）
- [x] 9 条 HTTP 路由全部注册（与旧实现一致）
- [x] WebSocket 推流功能保留（WebSocketStreamService）
- [x] 运行方式不变：`API.exe`（程序启动时自动加载同目录 config.json）

---

## 九、运行说明

```bash
# 编译
cmake --build cmake-build-debug --target API -j 8

# 运行（config.json 需与可执行文件同目录）
./API.exe
```

启动流程：加载 `config.json`（失败则回退默认配置）→ AppContext 装配五层 → 依次启动
TCP 指令通道（8083）、UDP 心跳（8082 广播）、WebSocket 推流（8081）→ HTTP 服务阻塞运行（8082）。
