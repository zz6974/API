# OpenSSL SDK 配置 - 使用 INTERFACE 库转发到实际的 OpenSSL 库文件

set(OPENSSL_SDK_INCLUDE "" CACHE PATH "OpenSSL SDK include directory")
set(OPENSSL_SDK_SSL_LIBRARY "" CACHE FILEPATH "OpenSSL SSL library path")
set(OPENSSL_SDK_CRYPTO_LIBRARY "" CACHE FILEPATH "OpenSSL Crypto library path")

# 验证必需的路径参数
if(NOT OPENSSL_SDK_INCLUDE)
    message(FATAL_ERROR "OPENSSL_SDK_INCLUDE must be specified!")
endif()

if(NOT EXISTS "${OPENSSL_SDK_INCLUDE}")
    message(FATAL_ERROR "OPENSSL_SDK_INCLUDE path does not exist: ${OPENSSL_SDK_INCLUDE}")
endif()

# Deleted:#option(OPENSSL_SDK_SSL_INCLUDE "")
# Deleted:#option(OPENSSL_SDK_SSL_LIBRARY "")
# Deleted:#option(OPENSSL_SDK_CRYPTO_INCLUDE "")
# Deleted:#option(OPENSSL_SDK_CRYPTO_LIBRARY "")
# Deleted:
# Deleted:set(OPENSSL_SDK_INCLUDE "" CACHE PATH "OpenSSL SDK include directory")
# Deleted:set(OPENSSL_SDK_SSL_LIBRARY "" CACHE FILEPATH "OpenSSL SSL library path")
# Deleted:set(OPENSSL_SDK_CRYPTO_LIBRARY "" CACHE FILEPATH "OpenSSL Crypto library path")
# Deleted:
# Deleted:include_directories(${OPENSSL_SDK_INCLUDE})
# Deleted:
# Deleted:add_library(ssl STATIC
# Deleted:    src/ssl.cpp
# Deleted:)
# Deleted:
# Deleted:add_library(crypto STATIC
# Deleted:    src/crypto.cpp
# Deleted:)
# Deleted:
# Deleted:target_include_directories(ssl PUBLIC ${OPENSSL_SDK_INCLUDE})
# Deleted:target_include_directories(crypto PUBLIC ${OPENSSL_SDK_INCLUDE})
# Deleted:
# Deleted:target_link_libraries(ssl PUBLIC ${OPENSSL_SDK_SSL_LIBRARY})
# Deleted:target_link_libraries(crypto PUBLIC ${OPENSSL_SDK_CRYPTO_LIBRARY})

# 创建 INTERFACE 库（不编译源码，只做转发）
add_library(ssl INTERFACE)
add_library(crypto INTERFACE)

# 设置包含目录
target_include_directories(ssl INTERFACE ${OPENSSL_SDK_INCLUDE})
target_include_directories(crypto INTERFACE ${OPENSSL_SDK_INCLUDE})

# 链接实际的 OpenSSL 库文件
if(OPENSSL_SDK_SSL_LIBRARY)
    if(EXISTS "${OPENSSL_SDK_SSL_LIBRARY}")
        target_link_libraries(ssl INTERFACE ${OPENSSL_SDK_SSL_LIBRARY})
        message(STATUS "OpenSSL SSL library: ${OPENSSL_SDK_SSL_LIBRARY}")
    else()
        message(WARNING "SSL library not found: ${OPENSSL_SDK_SSL_LIBRARY}")
    endif()
else()
    message(WARNING "OPENSSL_SDK_SSL_LIBRARY not specified")
endif()

if(OPENSSL_SDK_CRYPTO_LIBRARY)
    if(EXISTS "${OPENSSL_SDK_CRYPTO_LIBRARY}")
        target_link_libraries(crypto INTERFACE ${OPENSSL_SDK_CRYPTO_LIBRARY})
        message(STATUS "OpenSSL Crypto library: ${OPENSSL_SDK_CRYPTO_LIBRARY}")
    else()
        message(WARNING "Crypto library not found: ${OPENSSL_SDK_CRYPTO_LIBRARY}")
    endif()
else()
    message(WARNING "OPENSSL_SDK_CRYPTO_LIBRARY not specified")
endif()

message(STATUS "OpenSSL SDK configured successfully:")
message(STATUS "  Include: ${OPENSSL_SDK_INCLUDE}")
