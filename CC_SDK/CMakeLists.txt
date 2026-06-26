cmake_minimum_required(VERSION 2.8...3.5)


if(MSVC OR CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    add_compile_options(/utf-8)
endif()


if(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    message(WARNING "当前构建系统为 OpenHarmony")
    set(OS_OHOS ON PARENT_SCOPE)
else()
    set(OS_OHOS OFF PARENT_SCOPE)
endif()
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm|ARM|Armv[0-9]+")
    message(STATUS "Target architecture is ARM")
    message(STATUS "目标架构是 ${CMAKE_SYSTEM_PROCESSOR}")
    set(ARCH_ARM ON)
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64|ARM64")
        set(ARM64 ON PARENT_SCOPE)
    elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm|ARM")
        set(ARM32 ON PARENT_SCOPE)
    endif ()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686|x86_64|amd64|AMD64")
    message(STATUS "Target architecture is x86/x86_64")
    message(STATUS "目标架构是 x86/x86_64")
    set(ARCH_X86 ON PARENT_SCOPE)
else()
    message(WARNING "Unknown architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()
#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ENABLE_INSTALL_RULES OFF CACHE BOOL "Disable install rules for cppp-reiconv")
add_subdirectory(Environment/cppp-reiconv)
add_subdirectory(Environment/filesystem)
add_subdirectory(Environment/mirrors_nlohmann_json)
if( WIN32 AND NOT CYGWIN )
    add_definitions( -DOPENSSL_SYSNAME_WIN32 )
    add_definitions( -DWIN32_LEAN_AND_MEAN )
    add_definitions( -D_CRT_SECURE_NO_WARNINGS )

    # Fix for DEFINE_STACK_OF macro conflicts with Windows headers
    add_definitions( -DNDEBUG )

    if(BUILD_SHARED_LIBS)
        # avoid conflict: ocsp.h and wincrypt.h
        add_definitions( -D_WINDLL )
    endif()
endif()

option(OPENSSL_SDK_EN "Whether to use a custom openssl library" OFF)
if (OPENSSL_SDK_EN)
    # 使用自定义 OpenSSL SDK
    set(OPENSSL_SDK_INCLUDE "" CACHE PATH "OpenSSL SDK include directory")
    set(OPENSSL_SDK_SSL_LIBRARY "" CACHE FILEPATH "OpenSSL SSL library path")
    set(OPENSSL_SDK_CRYPTO_LIBRARY "" CACHE FILEPATH "OpenSSL Crypto library path")

    add_subdirectory(Environment/openssl_sdk)
else ()
    # 使用内置 openssl-cmake
    add_subdirectory(Environment/openssl-cmake)
endif ()

# ===== 资源配置：在编译目录下创建文件夹并运行 Python 脚本 =====
set(OUTPUT_RESOURCE_DIR ${CMAKE_BINARY_DIR}/Resources)
set(OUTPUT_RESOURCE_PAK_DIR ${CMAKE_BINARY_DIR}/bin/Resources)
set(INPUT_RESOURCE_DIR ${CMAKE_SOURCE_DIR}/CC_SDK_Resources/Static)
set(INPUT_RESOURCE_PAK_DIR ${CMAKE_SOURCE_DIR}/CC_SDK_Resources/ResPak)
set(CC_RESOURCE_JSON ${INPUT_RESOURCE_DIR})
set(CC_RESOURCE_PAK ${INPUT_RESOURCE_PAK_DIR})
get_filename_component(CC_RESOURCE_DIR ${CC_RESOURCE_JSON} DIRECTORY)

# 在编译目录下创建指定文件夹
file(MAKE_DIRECTORY ${OUTPUT_RESOURCE_DIR})
file(MAKE_DIRECTORY ${OUTPUT_RESOURCE_PAK_DIR})
file(MAKE_DIRECTORY ${INPUT_RESOURCE_DIR})
file(MAKE_DIRECTORY ${INPUT_RESOURCE_PAK_DIR})

# 查找 Python3
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    message(STATUS "Python3 found: ${Python3_EXECUTABLE}")

    # 定义 Python 脚本路径
    set(PYTHON_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/ResourceSystem/bundle_resources.py)
    set(PYTHON_SCRIPT_PAK ${CMAKE_CURRENT_SOURCE_DIR}/ResourceSystem/bundle_resources_pak.py)

    # 收集 INPUT_RESOURCE_DIR 下所有文件，内容变化时触发脚本重新运行
    file(GLOB_RECURSE RESOURCE_INPUT_FILES CONFIGURE_DEPENDS "${INPUT_RESOURCE_DIR}/*")
    file(GLOB_RECURSE RESOURCE_PAK_INPUT_FILES CONFIGURE_DEPENDS "${INPUT_RESOURCE_PAK_DIR}/*")

    # 生成的 C++ 文件路径
    set(GENERATED_CPP_FILE ${OUTPUT_RESOURCE_DIR}/CCResources.cpp)
    set(GENERATED_PAK_FILE ${OUTPUT_RESOURCE_PAK_DIR}/CCResources.pak)

    # ★ 检测资源文件变化：若任一输入比输出新，删除输出以触发重新生成
    set(NEED_REGEN_RESOURCES FALSE)
    foreach(res_file ${RESOURCE_INPUT_FILES})
        if(EXISTS ${GENERATED_CPP_FILE} AND res_file IS_NEWER_THAN ${GENERATED_CPP_FILE})
            set(NEED_REGEN_RESOURCES TRUE)
            break()
        endif()
    endforeach()
    if(NEED_REGEN_RESOURCES)
        file(REMOVE ${GENERATED_CPP_FILE})
        message(STATUS "Resource files changed, will regenerate ${GENERATED_CPP_FILE}")
    endif()

    set(NEED_REGEN_PAK FALSE)
    foreach(pak_file ${RESOURCE_PAK_INPUT_FILES})
        if(EXISTS ${GENERATED_PAK_FILE} AND pak_file IS_NEWER_THAN ${GENERATED_PAK_FILE})
            set(NEED_REGEN_PAK TRUE)
            break()
        endif()
    endforeach()
    if(NEED_REGEN_PAK)
        file(REMOVE ${GENERATED_PAK_FILE})
        message(STATUS "Resource PAK files changed, will regenerate ${GENERATED_PAK_FILE}")
    endif()

    # 添加自定义命令，在构建时运行 Python 脚本
    add_custom_command(
            OUTPUT ${GENERATED_CPP_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_RESOURCE_DIR}
            COMMAND ${Python3_EXECUTABLE} ${PYTHON_SCRIPT}
            ${CC_RESOURCE_JSON}
            ${GENERATED_CPP_FILE}
            DEPENDS ${PYTHON_SCRIPT}
            COMMENT "Bundling resources using Python script..."
            WORKING_DIRECTORY ${CC_RESOURCE_DIR}
    )

    # 创建自定义目标（用于手动触发）
    add_custom_target(RebuildResources
            COMMAND ${CMAKE_COMMAND} -E echo "Rebuilding resources..."
            COMMAND ${Python3_EXECUTABLE} ${PYTHON_SCRIPT}
            ${CC_RESOURCE_JSON}
            ${GENERATED_CPP_FILE}
            WORKING_DIRECTORY ${CC_RESOURCE_DIR}
            COMMENT "Manually rebuilding resources..."
            VERBATIM
    )

    add_custom_command(
            OUTPUT ${GENERATED_PAK_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_RESOURCE_DIR}
            COMMAND ${Python3_EXECUTABLE} ${PYTHON_SCRIPT_PAK}
            ${CC_RESOURCE_PAK}
            ${GENERATED_PAK_FILE}
            DEPENDS ${PYTHON_SCRIPT_PAK}
            COMMENT "Bundling resources pak using Python script..."
            WORKING_DIRECTORY ${CC_RESOURCE_DIR}
    )

    add_custom_target(RebuildPakResources
            COMMAND ${CMAKE_COMMAND} -E echo "Rebuilding pak resources..."
            COMMAND ${Python3_EXECUTABLE} ${PYTHON_SCRIPT_PAK}
            ${CC_RESOURCE_PAK}
            ${GENERATED_PAK_FILE}
            WORKING_DIRECTORY ${CC_RESOURCE_DIR}
            COMMENT "Manually rebuilding pak resources..."
            VERBATIM
    )
else()
    message(WARNING "Python3 not found, resource bundling will be skipped")
    # 如果找不到 Python，创建一个空的占位文件以避免编译错误
    set(GENERATED_CPP_FILE ${OUTPUT_RESOURCE_DIR}/CCResources.cpp)
    if(NOT EXISTS ${GENERATED_CPP_FILE})
        file(WRITE ${GENERATED_CPP_FILE} "// Empty resource file - Python not available\n")
    endif()
endif()

option(W_T_L_Arm "Window_To_Linux_Arm" OFF)
if (W_T_L_Arm)
    message(WARNING "Currently, it is cross-compiled for the Windows platform and the Linux platform for the Arm architecture")
endif ()

option(SerialPort "CC_SerialPort_OAF" OFF)
if (SerialPort)
    message(STATUS "CSerialPort ON")
    add_subdirectory(Environment/CSerialPort)
    set(SerialPort_STATIC
            src/Module/Comm/CCSerialPort.cpp
    )
endif ()

option(AUDIO_IO "CC_Audio_IO_OAF" OFF)
if (AUDIO_IO)
    message(STATUS "AUDIO_IO ON")
    set(AUDIO_IO_STATIC
            src/Module/Multimedia/CCAudioIO.cpp
    )
endif ()

option(PORTAUDIO "CC_PORTAUDIO_OAF" OFF)
if (PORTAUDIO)
    message(STATUS "PORTAUDIO ON")
    add_subdirectory(Environment/portaudio)
    set(portaudio_STATIC
            src/Module/Multimedia/CCAudio.cpp
    )
endif ()

option(FFMPEG "CC_FFMPEG_OAF" OFF)
if (FFMPEG)
    message(STATUS "FFMPEG ON")
    add_subdirectory(Environment/FFmpeg)
endif ()

option(SDL2 "CC_SDL2_OAF" OFF)
if (SDL2)
    message(STATUS "SDL2 ON")
    add_subdirectory(Environment/SDL2)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/Environment/SDL2/include)
endif ()

option(LIBRTC "CC_LIBRTC_OAF" OFF)
if(LIBRTC)
    add_subdirectory(Environment/libdatachannel)
endif()

option(LIBJUICE "CC_LIBJUICE_OAF" OFF)
if(LIBJUICE)
    if (NOT LIBRTC)
        add_subdirectory(Environment/libjuice)
    endif ()
endif()

option(SimpleGUI "CC_SimpleGUI_OAF" OFF)
if (SimpleGUI)
    message(STATUS "SimpleGUI ON")
    set(SimpleGUI_SOURCES
        src/SimpleGUI/UI/IWindow.cpp
        src/SimpleGUI/UI/UIWindow.cpp
        src/SimpleGUI/UI/GraphicsContext.cpp
        src/SimpleGUI/SimpleGUI.cpp
        src/SimpleGUI/Components/Renderer.cpp
    )
endif ()

option(GLFW "CC_GLFW_OAF" OFF)
if (GLFW)
    message(STATUS "GLFW ON")
    add_subdirectory(Environment/glfw)
    add_subdirectory(Environment/glad)
endif ()

option(MYSQL "CC_MYSQL" OFF)
if (MYSQL)
    message(STATUS "CC_MYSQL ON")
    add_subdirectory(Environment/MySql)
endif ()

option(RTSP "CC_RTSP" OFF)
if (RTSP)
    add_subdirectory(Environment/jrtplib)
    message(STATUS "CC_RTSP ON")
    set(CC_RTSP_Module
            src/Module/Comm/RTSP/CCRTSPServer.cpp
            Include/Module/Comm/RTSP/CCRTPReceiver.h
            src/Module/Comm/RTSP/CCRTPReceiver.cpp
            Include/Module/Comm/RTSP/CCRTPSender.h
            src/Module/Comm/RTSP/CCRTPSender.cpp
    )
endif ()

option(CURL "CC_CURL" OFF)
if (CURL)
    # 强制使用静态库
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
    set(BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries" FORCE)

    # 强制使用 OpenSSL
    set(CURL_USE_OPENSSL ON CACHE BOOL "Use OpenSSL" FORCE)


    # 根据 OPENSSL_SDK_EN 选择 OpenSSL 路径
    if(OPENSSL_SDK_EN)
        set(OPENSSL_ROOT_DIR "${OPENSSL_SDK_INCLUDE}" CACHE PATH "OpenSSL root dir" FORCE)
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_SDK_INCLUDE}" CACHE PATH "OpenSSL include" FORCE)
        message(STATUS "CC_CURL: Using custom OpenSSL SDK from ${OPENSSL_SDK_INCLUDE}")
    else()
        set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/Environment/openssl-cmake" CACHE PATH "OpenSSL root dir" FORCE)
        set(OPENSSL_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/Environment/openssl-cmake/include" CACHE PATH "OpenSSL include" FORCE)
        message(STATUS "CC_CURL: Using built-in OpenSSL from openssl-cmake")
    endif()

    # 关键：禁用 curl 的 CMake 导出功能，避免 ssl/crypto 目标导出问题
    set(CURL_ENABLE_EXPORT_TARGET OFF CACHE BOOL "Disable curl export target" FORCE)

    # 禁用可选的压缩库支持（避免依赖问题）
    set(CURL_ZSTD OFF CACHE BOOL "Disable Zstd support" FORCE)
    set(CURL_BROTLI OFF CACHE BOOL "Disable Brotli support" FORCE)
    set(USE_NGHTTP2 OFF CACHE BOOL "Disable HTTP/2 support" FORCE)
    set(USE_NGTCP2 OFF CACHE BOOL "Disable HTTP/3 ngtcp2 support" FORCE)
    set(USE_QUICHE OFF CACHE BOOL "Disable HTTP/3 quiche support" FORCE)
    set(USE_MSH3 OFF CACHE BOOL "Disable HTTP/3 msh3 support" FORCE)

    add_subdirectory(Environment/curl)
    message(STATUS "CC_CURL ON - Using built-in static OpenSSL (exports disabled)")
endif ()

option(ODBC "CC_ODBC" OFF)
if (ODBC)
    if (WIN32)
    else ()

    endif ()
    add_subdirectory(Environment/nanodbc)
    message(STATUS "CC_ODBC ON")
endif ()

add_subdirectory(Environment/zlib)
set(MINIZIP_SOURCES
        Environment/zlib/contrib/minizip/ioapi.c
        Environment/zlib/contrib/minizip/iowin32.c
        Environment/zlib/contrib/minizip/unzip.c
        Environment/zlib/contrib/minizip/zip.c
)
add_library(minizip STATIC ${MINIZIP_SOURCES})
target_include_directories(minizip PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Environment/zlib
        ${CMAKE_CURRENT_SOURCE_DIR}/Environment/zlib/contrib/minizip
)
target_link_libraries(minizip zlib)


set(INC_DIR ${CMAKE_CURRENT_LIST_DIR}/Include)
set(LINK_DIR ${CMAKE_CURRENT_LIST_DIR}/Lib)
set(LINK_DLL_DIR ${CMAKE_CURRENT_LIST_DIR}/Bin)
set(LIB_WX_D ${CMAKE_CURRENT_LIST_DIR}/Lib/wx/libwxmsw32ud.a)
set(LIB_WX_R ${CMAKE_CURRENT_LIST_DIR}/Lib/wx/libwxmsw32u.a)


set(CC_API_INC
        ${CMAKE_CURRENT_LIST_DIR}/Include
        ${CMAKE_CURRENT_LIST_DIR}/Include/basic
        ${CMAKE_CURRENT_LIST_DIR}/Environment/mirrors_nlohmann_json/include
        ${CMAKE_CURRENT_LIST_DIR}/Environment/mirrors_nlohmann_json/include/nlohmann
        ${CMAKE_CURRENT_LIST_DIR}/Include/CCServlet/SQL
        ${CMAKE_CURRENT_LIST_DIR}/Include/Module
        ${CMAKE_CURRENT_LIST_DIR}/Include/Module/File
        ${CMAKE_CURRENT_LIST_DIR}/Include/Module/Comm
        ${CMAKE_CURRENT_LIST_DIR}/Include/Module/Data
        ${CMAKE_CURRENT_LIST_DIR}/Include/Module/IO
        ${CMAKE_CURRENT_LIST_DIR}/Include/CCServlet
        ${CMAKE_CURRENT_LIST_DIR}/Environment/mysql-9.0.1/include
        ${CMAKE_CURRENT_LIST_DIR}/Environment/filesystem/include
        ${CMAKE_CURRENT_LIST_DIR}/Include/Module/Comm/WebSocket
        ${CMAKE_CURRENT_LIST_DIR}/Include/Module/Window
        ${CMAKE_CURRENT_LIST_DIR}/Include/Module/Comm/Socket
        ${CMAKE_CURRENT_LIST_DIR}/ResourceSystem
)

if (OPENSSL_SDK_EN)
    list(APPEND CC_API_INC
           ${OPENSSL_SDK_INCLUDE}
    )
else ()
    list(APPEND CC_API_INC
            ${CMAKE_CURRENT_LIST_DIR}/Environment/openssl-cmake/include
            ${CMAKE_CURRENT_LIST_DIR}/Environment/openssl-cmake/include/openssl
    )
endif ()

if (AUDIO_IO)
    list(APPEND CC_API_INC
            ${CMAKE_CURRENT_LIST_DIR}/Include/Module/Multimedia
    )
endif ()

if (PORTAUDIO)
    list(APPEND CC_API_INC
            ${CMAKE_CURRENT_LIST_DIR}/Include/Module/Multimedia
            ${CMAKE_CURRENT_LIST_DIR}/Include/Multimedia
            ${CMAKE_CURRENT_LIST_DIR}/Environment/portaudio/include
    )
endif ()

if (SerialPort)
    list(APPEND CC_API_INC ${CMAKE_CURRENT_LIST_DIR}/Environment/CSerialPort/include)
endif ()

if (FFMPEG)
    list(APPEND CC_API_INC ${CC_FFMPEG_INC})
endif ()

if (SDL2)
    list(APPEND CC_API_INC ${CMAKE_CURRENT_LIST_DIR}/Environment/SDL2/include)
endif ()

if (LIBRTC)
    list(APPEND CC_API_INC ${CMAKE_CURRENT_LIST_DIR}/Environment/libdatachannel/include)
    list(APPEND CC_API_INC ${CMAKE_CURRENT_LIST_DIR}/Include/Module/Comm/WebRTC)
    set(LIBRTC_STATIC src/Module/Comm/WebRTC/CCWebRTC.cpp)
endif ()

if (LIBJUICE)
    if (NOT LIBRTC)
        list(APPEND CC_API_INC ${CMAKE_CURRENT_LIST_DIR}/Environment/libjuice/include)
    endif ()
    set(CC_DataChannel
            src/Module/Comm/DataChannel/RTCPeerConnection.cpp
    )
endif ()

if (GLFW)
    list(APPEND CC_API_INC
            ${CMAKE_CURRENT_LIST_DIR}/Environment/glfw/include
            ${CMAKE_CURRENT_LIST_DIR}/Environment/glad/include
    )
endif ()

if (SimpleGUI)
    list(APPEND CC_API_INC
            ${CMAKE_CURRENT_LIST_DIR}/Include/SimpleGUI
    )
endif ()

if (MYSQL)
    list(APPEND CC_API_INC ${CC_MySql_INC})
    list(APPEND CC_API_INC Include/Environment/mysql)
    set(MYSQL_Module
            Include/Environment/mysql/CCMySql.cpp
    )
endif ()

if (RTSP)
    list(APPEND CC_API_INC ${J_RTP_LIB_INC} ${JThread_INC})
endif ()

if (CURL)
    list(APPEND CC_API_INC
            Environment/curl/include
    )
endif ()

if (ODBC)
    list(APPEND CC_API_INC
            Environment/nanodbc
            Environment/nanodbc/nanodbc
    )
endif ()

list(APPEND CC_API_INC ${CMAKE_CURRENT_LIST_DIR}/Environment/zlib/contrib/minizip)
list(APPEND CC_API_INC ${CMAKE_CURRENT_LIST_DIR}/Environment/zlib/contrib)
list(APPEND CC_API_INC ${CMAKE_CURRENT_LIST_DIR}/Environment/zlib)
list(APPEND CC_API_INC ${CMAKE_BINARY_DIR}/CC_SDK/Environment/zlib)
#list(APPEND CC_API_INC ${CMAKE_CURRENT_LIST_DIR}/Environment/zlib/include)
list(APPEND CC_API_INC ${CMAKE_CURRENT_LIST_DIR}/Include/Environment/zlib)
set(ZLIB_Module
        Include/Environment/zlib/CCZip.cpp
        Include/Environment/zlib/CCDataFormat.cpp
        Include/Environment/zlib/CCGZip.cpp
        Include/Environment/zlib/CCGZip.h
)


add_library(CC_API
        STATIC
        Include/basic/CCEncode.h
        Include/basic/CCByteArray.h
        Include/basic/CCString.h
        src/basic/CCObject.cpp
        src/basic/CCEncode.cpp
        src/basic/CCByteArray.cpp
        src/basic/CCString.cpp
        src/Module/File/CCFIleInStream.cpp
        src/Module/File/CCFileOutStream.cpp
        src/Module/Comm/Socket/CCSocket.cpp
        src/Module/Data/CCJSONObject.cpp
        src/Module/IO/CCThread.cpp
        src/Module/IO/CCProcess.cpp
        src/Module/IO/CCThreadPool.cpp
        src/Module/IO/CCTimeData.cpp
        src/Module/Comm/CCEpoll.cpp
        src/CCServlet/CCRequest.cpp
        src/CCServlet/CCResponse.cpp
        src/CCServlet/CCSQLite3.cpp
        src/CCServlet/CCWebServlet.cpp
        src/CCServlet/CCHttpClient.cpp
        src/CCServlet/CCHttpResolver.cpp
        src/Sys/wepoll.c
        src/Sys/sqlite3.c
        Include/Sys/sqlite3.h
        Include/Sys/sqlite3ext.h
        Include/basic/CCObject.h
        Include/basic/CC.h
        Include/CCAPI.h
        Include/basic/CCByte.h
        Include/basic/CCVector.h
        Include/basic/CCMap.h
        Include/basic/CCList.h
        Include/basic/CCQueue.h
        Include/basic/CCArray.h
        Include/Module/Comm/CCNetwork.h
        src/Module/Comm/CCNetwork.cpp
        Include/Module/File/CCFile.h
        src/Module/File/CCFile.cpp
        Include/basic/CCFunction.h
        Include/basic/CCAutoLock.h
        Include/basic/CCArrayList.h
        Include/basic/CCLinkedMap.h
        Include/basic/CCAllocator.h
        src/Module/Comm/WebSocket/base64.cpp
        src/Module/Comm/WebSocket/sha1_portable.cpp
        src/Module/Comm/WebSocket/string_helper.cpp
        src/Module/Comm/WebSocket/ws_endpoint.cpp
        src/Module/Comm/WebSocket/ws_packet.cpp
        Include/Module/Comm/WebSocket/CCWebSocket.h
        src/Module/Comm/WebSocket/CCWebSocket.cpp
#        Environment/openssl-cmake/env/openssl/applink.h
        src/Module/Window/CCApplication.cpp
        Include/basic/ConcurrentQueue.h
        Include/basic/ConcurrentMap.h
        Include/basic/ConcurrentLinkedList.h
        src/basic/CCTimer.cpp
        src/Module/File/CCLogger.cpp
        ${LIBRTC_STATIC}
        src/DataModule/NetSqlite.cpp
        Include/DataModule/NetSqlite.h
        Include/Module/Window/CCSystem.h
        src/Module/Window/CCSystem.cpp
        Include/basic/StreamHandler.h
        Include/Multimedia/CCMiniMP3.cpp
        Include/Multimedia/CCMiniMP3.h
        ${ZLIB_Module}
        ${SerialPort_STATIC}
        ${portaudio_STATIC}
        ${SimpleGUI_SOURCES}
        Include/TL/Array_CTL.h
        Include/TL/Map_CTL.h
        ${AUDIO_IO_STATIC}
        src/Module/Comm/Socket/CCDatagramSocket.cpp
        src/Module/Comm/Socket/CCServerSocket.cpp
        src/Module/Comm/Socket/CCClientSocket.cpp
        ${CC_DataChannel}
        src/Module/Comm/OpenSSL/CCSSL.cpp
        src/CCServlet/HttpSocketChannel.cpp
        src/Module/IO/EventLoop.cpp
        Include/Module/IO/EventLoop.h
        src/basic/CCMemoryPool.cpp
        Include/basic/CCMemoryPool.h
        src/Module/Data/Persistent.cpp
        Include/Module/Data/Persistent.h
        src/basic/CCClassFactory.cpp
        Include/basic/CCClassFactory.h
        src/basic/CCClassRegister.cpp
        ${MYSQL_Module}
        ${CC_RTSP_Module}
        src/Module/Data/JSON/JSONObject.cpp
        src/Module/Data/JSON/Parser.cpp
        src/basic/CC.cpp
        Include/Module/Comm/SMTP/CCSMTP.h
        src/Module/Comm/SMTP/CCSMTP.cpp
        src/Module/Comm/Socket/EPSocket.cpp
        Include/Module/Data/CCSharedMemory.h
        src/Module/Data/CCSharedMemory.cpp
        src/Module/Data/CCProcessQueue.cpp
        ResourceSystem/CCResources.h
        ${OUTPUT_RESOURCE_DIR}/CCResources.cpp
        ResourceSystem/CCPak_Reader.cpp
        ${GENERATED_PAK_FILE}
        src/Module/Comm/MCP/CCMCPServer.cpp
)

if (OPENSSL_SDK_EN)
    list(${CC_API} APPEND
            Environment/openssl-cmake/env/openssl/applink.h
    )
else ()
    list(${CC_API} APPEND
            Environment/openssl-cmake/env/openssl/applink.h
    )
endif ()

target_include_directories(CC_API PUBLIC
        ${CC_API_INC}
)
if(${OS_OHOS})
    target_link_libraries(CC_API PUBLIC libhilog_ndk.z.so)
endif()
if(CMAKE_HOST_UNIX)
    target_link_libraries(CC_API PUBLIC
            nlohmann_json
            libcppp-reiconv.static
            crypto ssl
    )
    if (AUDIO_IO)
        target_link_libraries(CC_API PUBLIC asound)
    endif ()
elseif(CMAKE_HOST_WIN32)
    target_link_libraries(CC_API PUBLIC
            nlohmann_json
            libcppp-reiconv.static
            crypto ssl
    )
    if (NOT CMAKE_SYSTEM_NAME STREQUAL "OHOS" AND NOT W_T_L_Arm)
        target_link_libraries(CC_API PUBLIC shlwapi shell32 Pdh iphlpapi winmm mswsock)
    endif ()
    add_compile_definitions(UNICODE _UNICODE) # 强制使用Unicode
else()


endif()
if (PORTAUDIO)
    target_link_libraries(CC_API PUBLIC PortAudio)
endif ()
if (SerialPort)
    target_link_libraries(CC_API PUBLIC libcserialport)
endif ()
if (FFMPEG)
    target_link_libraries(CC_API PUBLIC CC_FFmpeg_API)
endif ()
if (SDL2)
    target_link_libraries(CC_API PUBLIC
            SDL2::SDL2
            SDL2main
    )
endif ()
if (LIBRTC)
    target_link_libraries(CC_API PUBLIC datachannel)
endif ()
if (LIBJUICE)
    if (NOT LIBRTC)
        target_link_libraries(CC_API PUBLIC LibJuice::LibJuiceStatic)
    endif ()
    add_definitions(-DLIBJUICE_INC)
    target_compile_definitions(CC_API PUBLIC LIBJUICE_INC)
endif ()
if (GLFW)
    target_link_libraries(CC_API PUBLIC glfw glad)
endif ()
if (MYSQL)
    target_link_libraries(CC_API PUBLIC CC_MySql_API)
    #        if (CMAKE_HOST_WIN32)
    #            target_compile_definitions(CC_API PRIVATE _WINSOCKAPI_)
    #        endif ()
    target_compile_definitions(CC_API PRIVATE MY_SQL_BOOL)
endif ()
if (SimpleGUI)
    if(CMAKE_HOST_UNIX)
        target_link_libraries(CC_API PUBLIC GL glfw X11)
    elseif(CMAKE_HOST_WIN32)
        target_link_libraries(CC_API PUBLIC opengl32 dwmapi glfw)
    else()

    endif ()
endif ()
if (RTSP)
    target_link_libraries(CC_API PUBLIC jrtplib-static)
endif ()

if (CURL)
    target_link_libraries(CC_API PUBLIC libcurl)
endif ()

if (ODBC)
    target_link_libraries(CC_API PUBLIC nanodbc)
endif ()

target_link_libraries(CC_API PUBLIC zlib minizip ghcFilesystem::ghc_filesystem)




