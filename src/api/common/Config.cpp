#include "api/common/Config.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace api::common {

Result<ServerConfig> ServerConfig::LoadFromJson(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return Result<ServerConfig>::Fail(ErrorCode::kIoError, "无法打开配置文件: " + filePath);
    }
    try {
        nlohmann::json json;
        file >> json;

        ServerConfig config;
        config.httpAddress          = json.value("httpAddress", config.httpAddress);
        config.httpPort             = json.value("httpPort", config.httpPort);
        config.tcpPort              = json.value("tcpPort", config.tcpPort);
        config.wsAddress            = json.value("wsAddress", config.wsAddress);
        config.wsPort               = json.value("wsPort", config.wsPort);
        config.photoDir             = json.value("photoDir", config.photoDir);
        config.photoTimeoutMs       = json.value("photoTimeoutMs", config.photoTimeoutMs);
        config.registerTimeoutMs    = json.value("registerTimeoutMs", config.registerTimeoutMs);
        config.onlineTimeoutSeconds = json.value("onlineTimeoutSeconds", config.onlineTimeoutSeconds);
        config.account              = json.value("account", config.account);
        config.password             = json.value("password", config.password);
        return Result<ServerConfig>::Ok(config);
    } catch (const std::exception& e) {
        return Result<ServerConfig>::Fail(ErrorCode::kInvalidArgument,
                                          std::string("配置解析失败: ") + e.what());
    }
}

} // namespace api::common
