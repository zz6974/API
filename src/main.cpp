#include "api/AppContext.h"
#include "api/common/Config.h"
#include "api/common/Logger.h"

int main() {
    system("chcp 65001");

    api::common::Logger::Instance().SetLevel(api::common::Logger::Level::kInfo);

    // 加载运行配置；失败时回退默认配置
    auto config = api::common::ServerConfig::LoadFromJson("config.json");
    if (config.IsFail()) {
        LOG_WARN("加载 config.json 失败(%s)，使用默认配置", config.Message().c_str());
        config = api::common::Result<api::common::ServerConfig>::Ok(
            api::common::ServerConfig::Default());
    }

    api::AppContext app;
    if (!app.Init(config.Value())) {
        LOG_ERROR("应用初始化失败");
        return 1;
    }

    app.Run();
    return 0;
}
