# API 摄像头管理服务端

基于 C++17 的摄像头管理服务端，提供 HTTP API、摄像头 TCP 长连接、UDP 心跳和 WebSocket 实时帧推送。项目采用分层架构和端口-适配器模式，业务服务通过接口依赖基础设施实现。

## 功能概览

- 摄像头登录认证与设备 ID 分配
- 查询在线设备、获取实时 JPEG 帧、控制推流
- 远程拍照，并保存和查询照片
- 通过 TCP 与摄像头终端进行指令、帧和照片数据传输
- 通过 UDP 心跳跟踪设备在线状态
- 通过 WebSocket 向前端推送实时帧
- 使用 `config.json` 配置网络地址、端口、超时和照片目录

## 架构

代码按职责分层：

- **common**：统一错误码、`Result<T>`、日志和配置
- **domain**：协议常量和领域模型
- **application**：摄像头、照片和认证用例，以及端口接口
- **web**：HTTP 路由、控制器和服务器封装
- **infrastructure**：TCP/UDP/WebSocket 网络服务、照片文件存储和 ID 分配
- **AppContext**：组合根，负责依赖装配和服务生命周期

详细说明见 [API_ARCHITECTURE.md](API_ARCHITECTURE.md)、[DESIGN.md](DESIGN.md) 和 [REFACTOR_CHANGES.md](REFACTOR_CHANGES.md)。

## 通信端口

| 端口 | 协议 | 用途 |
|---|---|---|
| 8082 | HTTP | 客户端 API |
| 8082 | UDP | 摄像头心跳 |
| 8083 | TCP | 摄像头注册、指令、视频帧和照片数据 |
| 8081 | WebSocket | 实时帧推送 |

以上是当前配置示例中的端口，实际运行值由 `config.json` 决定。请确保服务器防火墙和网络策略允许相应流量，并让摄像头终端连接到正确的服务器地址。

## HTTP API

| 方法 | 路径 | 说明 |
|---|---|---|
| `POST` | `/api/login` | 登录认证 |
| `GET` | `/api/GetID` | 分配摄像头 ID |
| `GET` | `/api/camera/list` | 获取在线摄像头列表 |
| `GET` | `/api/camera/{id}/frame` | 获取摄像头最新 JPEG 帧 |
| `POST` | `/api/camera/{id}/photo` | 拍照并保存，返回照片信息 |
| `POST` | `/api/camera/{id}/stream/start` | 开始推流 |
| `POST` | `/api/camera/{id}/stream/stop` | 停止推流 |
| `GET` | `/api/photo/{photoId}` | 获取照片 JPEG 数据 |
| `GET` | `/api/photo/list/{deviceId}` | 查询设备照片列表 |

登录请求示例：

```http
POST /api/login
Content-Type: application/json

{"Account":"your-account","Password":"your-password"}
```

JPEG 接口返回二进制图像数据；其他响应格式及错误码映射可参考架构文档中的路由说明。

## 配置

程序从可执行文件同目录的 `config.json` 读取配置。仓库中的配置仅供开发参考；部署时请根据实际网络环境修改地址、端口和认证信息。示例：

```json
{
  "httpAddress": "0.0.0.0",
  "httpPort": 8082,
  "tcpPort": 8083,
  "wsAddress": "0.0.0.0",
  "wsPort": 8081,
  "photoDir": "./photos",
  "photoTimeoutMs": 5000,
  "registerTimeoutMs": 5000,
  "onlineTimeoutSeconds": 30,
  "account": "replace-me",
  "password": "replace-me"
}
```

若配置文件无法加载，程序会回退到代码中的默认配置。请勿在公开环境中使用示例账号密码；生产部署应使用强凭据，并避免将真实秘密提交到版本控制。

## 构建与运行

项目使用 CMake，仓库包含 `CC_SDK` 子项目。需要支持 C++17 的编译器和 CMake；具体平台依赖以项目 CMake 配置为准。

```bash
# 配置
cmake -S . -B cmake-build-debug

# 构建服务端
cmake --build cmake-build-debug --target API -j 8
```

将 `config.json` 放到生成的可执行文件同目录后运行：

```bash
# Windows
API.exe

# Linux / macOS（以实际生成路径为准）
./API
```

启动时服务端加载配置并初始化各层组件，随后启动 TCP、UDP 心跳和 WebSocket 服务，再运行 HTTP 服务。照片文件保存在 `photoDir` 指定的目录中。

## 项目目录

```text
include/api/    公共、领域、应用、Web 和基础设施接口
src/            程序入口及组合根实现
CC_SDK/         项目使用的 C++ SDK 和构建依赖
config.json     本地运行配置
photos/         运行时照片存储目录
```

## 安全提示

- 当前 HTTP 接口的认证和传输安全应结合部署环境评估；不要将服务直接暴露到不可信网络。
- 修改默认账号密码，并通过网络隔离、防火墙或可信代理限制服务访问。
- 不要在仓库中提交生产环境凭据或包含敏感信息的配置文件。

## 相关文档

- [架构说明](API_ARCHITECTURE.md)
- [整体设计](DESIGN.md)
- [重构变更记录](REFACTOR_CHANGES.md)
