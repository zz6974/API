#pragma once
#include <string>
#include "api/common/Result.h"

namespace api::common {

/**
 * @brief 服务端运行配置
 *
 * 从 config.json 加载，替代散落的硬编码。
 */
struct ServerConfig {
    // ---- HTTP ----
    std::string httpAddress = "0.0.0.0"; ///< HTTP 监听地址（0.0.0.0 表示所有网卡）
    int httpPort = 8082;                 ///< HTTP 监听端口（客户端 API）

    // ---- TCP 指令通道 ----
    int tcpPort = 8083;                  ///< TCP 监听端口（摄像头长连接）

    // ---- WebSocket 推流 ----
    std::string wsAddress = "0.0.0.0";  ///< WS 监听地址
    int wsPort = 8081;                   ///< WS 监听端口（前端推流）

    // ---- 存储 ----
    std::string photoDir = "./photos";  ///< 照片存储目录

    // ---- 超时（毫秒 / 秒）----
    int photoTimeoutMs = 5000;           ///< 拍照等待回传超时（毫秒）
    int registerTimeoutMs = 5000;        ///< 终端注册等待超时（毫秒）
    int onlineTimeoutSeconds = 30;       ///< 心跳在线判定超时（秒）

    // ---- 认证 ----
    std::string account = "123456";     ///< 终端登录账号
    std::string password = "123456";    ///< 终端登录密码

    /**
     * @brief 默认配置
     * @return 保证缺配置也能跑的默认值
     */
    static ServerConfig Default() { return {}; }

    /**
     * @brief 从 JSON 文件加载配置
     * @param filePath 配置文件路径
     * @return 加载后的配置；文件缺失或解析失败返回 Fail（调用方自行回退默认值）
     */
    static Result<ServerConfig> LoadFromJson(const std::string& filePath);
};

} // namespace api::common
