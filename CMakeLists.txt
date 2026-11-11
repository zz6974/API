cmake_minimum_required(VERSION 4.2)
project(API)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Qt6：仅含 UI 的目标开启 moc/uic 自动处理（见 api_infrastructure）
# ============================================================
if(EXISTS "D:/qt/6.8.3/mingw_64")
    list(APPEND CMAKE_PREFIX_PATH "D:/qt/6.8.3/mingw_64")
endif()
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_subdirectory(CC_SDK) # 先构建 SDK，其导出的 CC_API 目标被各层链接

if(DEFINED ENV{OpenCV_DIR})
    set(OpenCV_DIR $ENV{OpenCV_DIR}) # 从环境变量接 OpenCV 安装路径
endif()
find_package(OpenCV REQUIRED)

set(API_INC_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(NLOHMANN_INC ${CMAKE_CURRENT_SOURCE_DIR}/CC_SDK/Environment/mirrors_nlohmann_json/include)

# 分层构建顺序：每层只链接下一层，依赖方向在编译期被强制
# api_common -> api_domain -> api_application -> (api_web | api_infrastructure) -> API

# ============================================================
# 1. common 公共层：Result / ErrorCode / Logger / Config
# ============================================================
add_library(api_common STATIC
    src/api/common/Config.cpp
)
target_include_directories(api_common PUBLIC
    ${API_INC_DIR}
    ${CC_API_INC}
    ${NLOHMANN_INC}
)
target_link_libraries(api_common PUBLIC CC_API)

# ============================================================
# 2. domain 领域层：协议 / 领域模型（纯头文件，不依赖任何层）
# ============================================================
add_library(api_domain INTERFACE)
target_include_directories(api_domain INTERFACE ${API_INC_DIR})
target_link_libraries(api_domain INTERFACE api_common)

# ============================================================
# 3. application 应用层：端口接口 + 用例服务
# ============================================================
add_library(api_application STATIC
    src/api/app/service/CameraService.cpp
    src/api/app/service/PhotoService.cpp
    src/api/app/service/AuthService.cpp
)
target_include_directories(api_application PUBLIC ${API_INC_DIR})
target_link_libraries(api_application PUBLIC api_domain)

# ============================================================
# 4. web 接口层：路由 / 控制器 / HTTP 服务器封装
# ============================================================
add_library(api_web STATIC
    src/api/web/http/Router.cpp
    src/api/web/http/BaseController.cpp
    src/api/web/http/HttpServer.cpp
    src/api/web/controller/CameraController.cpp
    src/api/web/controller/PhotoController.cpp
    src/api/web/controller/AuthController.cpp
    src/api/web/controller/IdController.cpp
)
target_include_directories(api_web PUBLIC
    ${API_INC_DIR}
    ${CC_API_INC}
    ${NLOHMANN_INC}
)
target_link_libraries(api_web PUBLIC api_application CC_API)

# ============================================================
# 5. infrastructure 基础设施层：网络 / 存储 / ID 分配具体实现
# ============================================================
add_library(api_infrastructure STATIC
    src/api/infra/net/TcpCameraChannel.cpp
    src/api/infra/net/CameraChannelManager.cpp
    src/api/infra/net/UdpHeartbeatService.cpp
    src/api/infra/net/WebSocketStreamService.cpp
    src/api/infra/persistence/FilePhotoRepository.cpp
    src/api/infra/id/SlotIdAllocator.cpp
        src/UI/cameraui.cpp
        include/UI/cameraui.h
        src/UI/cameraui.ui
)
# 仅本目标包含 Q_OBJECT / .ui 文件，在此开启 Qt 自动生成处理
set_target_properties(api_infrastructure PROPERTIES
        AUTOGEN_BUILD_DIR "${CMAKE_BINARY_DIR}/autogen_files"
)
set_target_properties(api_infrastructure PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)
target_include_directories(api_infrastructure PUBLIC
    ${API_INC_DIR}
    ${CC_API_INC}
)
target_link_libraries(api_infrastructure PUBLIC api_application CC_API Qt6::Widgets)

# ============================================================
# 6. 可执行程序：组合根装配
# ============================================================
add_executable(API
    src/main.cpp
    src/api/AppContext.cpp
)
target_include_directories(API PUBLIC
    ${API_INC_DIR}
    ${CC_API_INC}
)
target_link_libraries(API PRIVATE
    api_web
    api_infrastructure
    ${OpenCV_LIBS}
)
