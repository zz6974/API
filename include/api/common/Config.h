#pragma once
#include <string>
#include "api/common/Result.h"

namespace api::common {

// 服务端运行配置，从 config.json 加载，替代散落的硬编码
struct ServerConfig {
    // HTTP
    std::string httpAddress = "0.0.0.0";
    int httpPort = 8082;

    // TCP 指令通道
    int tcpPort = 8083;

    // WebSocket 推流
    std::string wsAddress = "0.0.0.0";
    int wsPort = 8081;

    // 存储
    std::string photoDir = "./photos";

    // 超时（毫秒 / 秒）
    int photoTimeoutMs = 5000;
    int registerTimeoutMs = 5000;
    int onlineTimeoutSeconds = 30;

    // 认证
    std::string account = "123456";
    std::string password = "123456";

    static ServerConfig Default() { return {}; }
    static Result<ServerConfig> LoadFromJson(const std::string& filePath);
};

} // namespace api::common
