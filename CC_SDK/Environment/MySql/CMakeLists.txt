cmake_minimum_required(VERSION 3.0...4.10)



if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/bin")
    # 文件存在时执行的操作
    message(STATUS "MySql Bin exists")
else()
    # 文件不存在时执行的操作
    message(STATUS "MySql Bin File does not exist")
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/file.zip")
        message(STATUS "Start downloading MySql resources...")
        file(DOWNLOAD "https://files.zohopublic.com.cn/public/workdrive-public/download/yjkvsa6ffc5f2ee2d44af9359efe87b835ec0?x-cli-msg=%7B%22linkId%22%3A%221p0DT3OmasW-32jEX%22%2C%22isFileOwner%22%3Afalse%2C%22version%22%3A%221.0%22%2C%22isWDSupport%22%3Afalse%7D" "${CMAKE_CURRENT_LIST_DIR}/file.zip"
                TIMEOUT 900
                TLS_VERIFY ON)
    endif ()

    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf "${CMAKE_CURRENT_LIST_DIR}/file.zip"
            WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
    message(STATUS "MySql resource installation is complete")
endif()


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(MYSQL_BOOL=1)

set(CC_MySql_INC
        ${CMAKE_CURRENT_LIST_DIR}/include
        PARENT_SCOPE
)

add_library(CC_MySql_API STATIC
        ${CMAKE_CURRENT_LIST_DIR}/src/A.cc
)
if (WIN32)
    message(STATUS "MySql Windows")
    target_include_directories(CC_MySql_API PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/include
    )
    if(MSVC OR CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_link_libraries(CC_MySql_API PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/libmysql.lib
        )
    else ()
        target_link_libraries(CC_MySql_API PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/libmysql.dll
        )
    endif()

    add_custom_command(TARGET CC_MySql_API POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/libmysql.dll
            ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/libssl-3-x64.dll
            ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/libcrypto-3-x64.dll
            ${CMAKE_BINARY_DIR}/bin
            COMMENT "Copying MySql DLLs to bin directory"
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm|ARM")
        target_include_directories(CC_MySql_API PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/include
        )
        target_link_libraries(CC_MySql_API PUBLIC

        )
    elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64|ARM64")
        target_include_directories(CC_MySql_API PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/include
        )
        target_link_libraries(CC_MySql_API PUBLIC

        )
    endif()
elseif (UNIX)
    message(STATUS "MySql Linux")
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64|ARM64")
            message(STATUS "MySql target architecture is ARM64")
            target_include_directories(CC_MySql_API PUBLIC
                    ${CMAKE_CURRENT_LIST_DIR}/include
            )
            target_link_libraries(CC_MySql_API PUBLIC
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libmysqlclient.so.21.2.44
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libcrypto.so.3
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libssl.so.3
            )
            add_custom_command(TARGET CC_MySql_API POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libmysqlclient.so.21.2.44
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libcrypto.so.3
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libssl.so.3
                    ${CMAKE_BINARY_DIR}/bin
                    COMMENT "Copying MySql DLLs to bin directory"
            )
        elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64|AMD64")
            message(STATUS "MySql target architecture is X86_64")
            target_include_directories(CC_MySql_API PUBLIC
                    ${CMAKE_CURRENT_LIST_DIR}/include
            )
            target_link_libraries(CC_MySql_API PUBLIC
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/X86/libmysqlclient.so.21.2.42
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/X86/libcrypto.so.3
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/X86/libssl.so.3
            )
            add_custom_command(TARGET CC_MySql_API POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/X86/libmysqlclient.so.21.2.42
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/X86/libcrypto.so.3
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/X86/libssl.so.3
                    ${CMAKE_BINARY_DIR}/bin
                    COMMENT "Copying MySql DLLs to bin directory"
            )
        endif()
    else ()
        if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm|ARM")
            message(STATUS "MySql target architecture is ARMv7")
            if(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
                target_include_directories(CC_MySql_API PUBLIC
                        ${CMAKE_CURRENT_LIST_DIR}/include
                )
                target_link_libraries(CC_MySql_API PUBLIC

                )
                add_custom_command(TARGET CC_MySql_API POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different

                        ${CMAKE_BINARY_DIR}/bin
                        COMMENT "Copying MySql DLLs to bin directory"
                )
            else()
                target_include_directories(CC_MySql_API PUBLIC
                        ${CMAKE_CURRENT_LIST_DIR}/include
                )
                target_link_libraries(CC_MySql_API PUBLIC
#                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavcodec.so.61.19.101
#                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavdevice.so.61.3.100
#                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavfilter.so.10.4.100
#                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavformat.so.61.7.100
#                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavutil.so.59.39.100
#                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libpostproc.so.58.3.100
#                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libswresample.so.5.3.100
#                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libswscale.so.8.3.100
#                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/ld-linux.so.3
#                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/ld-linux-armhf.so.3
                )
            endif()
        endif()
    endif()
endif()

if (CMAKE_HOST_WIN32)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    add_custom_command(TARGET CC_MySql_API POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:CC_MySql_API>
            ${CMAKE_BINARY_DIR}/bin
            COMMENT "Copying CC_MySql_API library to bin directory"
    )
endif ()

