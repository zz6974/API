cmake_minimum_required(VERSION 3.0...3.5)



if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/bin")
    # 文件存在时执行的操作
    message(STATUS "FFMpeg Bin exists")
else()
    # 文件不存在时执行的操作
    message(STATUS "FFMpeg Bin File does not exist")
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/file.zip")
        message(STATUS "Start downloading FFMpeg resources...")
        # https://files.zohopublic.com.cn/public/workdrive-public/download/05fgz6009e26d094048c9a96f6a6804e137ce?x-cli-msg=%7B%22linkId%22%3A%221p0DT3Omndk-32jEX%22%2C%22isFileOwner%22%3Afalse%2C%22version%22%3A%221.0%22%2C%22isWDSupport%22%3Afalse%7D
        file(DOWNLOAD "https://files.zohopublic.com.cn/public/workdrive-public/download/05fgz6009e26d094048c9a96f6a6804e137ce?x-cli-msg=%7B%22linkId%22%3A%221p0DT3Omndk-32jEX%22%2C%22isFileOwner%22%3Afalse%2C%22version%22%3A%221.0%22%2C%22isWDSupport%22%3Afalse%7D" "${CMAKE_CURRENT_LIST_DIR}/file.zip"
                TIMEOUT 900
                TLS_VERIFY ON)
    endif ()

    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf "${CMAKE_CURRENT_LIST_DIR}/file.zip"
            WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
    message(STATUS "FFMpeg resource installation is complete")
endif()


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CC_FFMPEG_INC
        ${CMAKE_CURRENT_LIST_DIR}/include/Windows/include
        ${CMAKE_CURRENT_LIST_DIR}/src
        PARENT_SCOPE
)



add_library(CC_FFmpeg_API STATIC
        ${CMAKE_CURRENT_LIST_DIR}/src/CCMP3.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/CCFFmpeg.cpp
        src/CCPCM.h
)
if (WIN32)
    message(STATUS "FFMPEG Windows")
    target_include_directories(CC_FFmpeg_API PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/include/Windows/include
    )
    if(MSVC OR CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_link_libraries(CC_FFmpeg_API PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/Lib/avcodec.lib
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/Lib/avdevice.lib
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/Lib/avfilter.lib
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/Lib/avformat.lib
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/Lib/avutil.lib
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/Lib/swresample.lib
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/Lib/swscale.lib
        )
    else ()
        target_link_libraries(CC_FFmpeg_API PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/avcodec-62.dll
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/avdevice-62.dll
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/avfilter-11.dll
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/avformat-62.dll
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/avutil-60.dll
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/swresample-6.dll
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/swresample.dll
                ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/swscale-9.dll
        )
    endif()
    add_custom_command(TARGET CC_FFmpeg_API POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/avcodec-62.dll
            ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/avdevice-62.dll
            ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/avfilter-11.dll
            ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/avformat-62.dll
            ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/avutil-60.dll
            ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/swresample-6.dll
            ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/swresample.dll
            ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/swscale-9.dll
            ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/ffmpeg.exe
            ${CMAKE_CURRENT_LIST_DIR}/bin/Windows/ffprobe.exe
            ${CMAKE_BINARY_DIR}/bin
            COMMENT "Copying FFmpeg DLLs to bin directory"
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm|ARM")
        target_include_directories(CC_FFmpeg_API PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/include/OHOS/Armv7/include
        )
        target_link_libraries(CC_FFmpeg_API PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/Armv7/libavcodec.so.60.3.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/Armv7/libavdevice.so.60.1.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/Armv7/libavfilter.so.9.3.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/Armv7/libavformat.so.60.3.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/Armv7/libswresample.so.4.10.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/Armv7/libswscale.so.7.1.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/Armv7/libavutil.so.58.2.100
        )
    elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64|AMD64")
        target_include_directories(CC_FFmpeg_API PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/include/OHOS/x86/include
        )
        target_link_libraries(CC_FFmpeg_API PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/x86_64/libavcodec.so.60.3.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/x86_64/libavdevice.so.60.1.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/x86_64/libavfilter.so.9.3.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/x86_64/libavformat.so.60.3.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/x86_64/libswresample.so.4.10.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/x86_64/libswscale.so.7.1.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/x86_64/libavutil.so.58.2.100
        )
    elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64|ARM64")
        target_include_directories(CC_FFmpeg_API PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/include/OHOS/Arm64/include
        )
        target_link_libraries(CC_FFmpeg_API PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/Arm64/libavcodec.so.60.3.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/Arm64/libavdevice.so.60.1.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/Arm64/libavfilter.so.9.3.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/Arm64/libavformat.so.60.3.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/Arm64/libswresample.so.4.10.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/Arm64/libswscale.so.7.1.100
                ${CMAKE_CURRENT_LIST_DIR}/bin/OHOS/Arm64/libavutil.so.58.2.100
        )
    endif()
elseif (UNIX)
    message(STATUS "FFMPEG Linux")
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64|ARM64")
            message(STATUS "ffmpeg target architecture is ARM64")
            target_include_directories(CC_FFmpeg_API PUBLIC
                    ${CMAKE_CURRENT_LIST_DIR}/include/Linux/Arm64/include
            )
            target_link_libraries(CC_FFmpeg_API PUBLIC
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libavcodec.so.62
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libavdevice.so.62
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libavfilter.so.11
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libavformat.so.62
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libavutil.so.60
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libswresample.so.6
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libswscale.so.9
            )
            add_custom_command(TARGET CC_FFmpeg_API POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libavcodec.so.62
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libavdevice.so.62
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libavfilter.so.11
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libavformat.so.62
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libavutil.so.60
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libswresample.so.6
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/libswscale.so.9
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/ffmpeg
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Arm64/ffprobe
                    ${CMAKE_BINARY_DIR}/bin
                    COMMENT "Copying FFmpeg DLLs to bin directory"
            )
        elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64|AMD64")
            message(STATUS "ffmpeg target architecture is X86_64")
            target_include_directories(CC_FFmpeg_API PUBLIC
                    ${CMAKE_CURRENT_LIST_DIR}/include/Linux/x86/include
            )
            target_link_libraries(CC_FFmpeg_API PUBLIC
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/libavcodec.so.62
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/libavdevice.so.62
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/libavfilter.so.11
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/libavformat.so.62
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/libavutil.so.60
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/libswresample.so.6
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/libswscale.so.9
            )
            add_custom_command(TARGET CC_FFmpeg_API POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/libavcodec.so.62
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/libavdevice.so.62
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/libavfilter.so.11
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/libavformat.so.62
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/libavutil.so.60
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/libswresample.so.6
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/libswscale.so.9
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/ffmpeg
                    ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/x86/ffprobe
                    ${CMAKE_BINARY_DIR}/bin
                    COMMENT "Copying FFmpeg DLLs to bin directory"
            )
        endif()
    else ()
        if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm|ARM")
            message(STATUS "ffmpeg target architecture is ARMv7")
            if(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
                target_include_directories(CC_FFmpeg_API PUBLIC
                        ${CMAKE_CURRENT_LIST_DIR}/include/OHOS/Armv7/include
                )
                target_link_libraries(CC_FFmpeg_API PUBLIC
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavcodec.so.61.19.101
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavdevice.so.61.3.100
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavfilter.so.10.4.100
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavformat.so.61.7.100
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavutil.so.59.39.100
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libpostproc.so.58.3.100
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libswresample.so.5.3.100
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libswscale.so.8.3.100
                )
                add_custom_command(TARGET CC_FFmpeg_API POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavcodec.so
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavdevice.so
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavfilter.so
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavformat.so
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavutil.so
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libswresample.so
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libpostproc.so
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libswscale.so
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libx264.so
                        ${CMAKE_BINARY_DIR}/bin
                        COMMENT "Copying FFmpeg DLLs to bin directory"
                )
            else()
                target_include_directories(CC_FFmpeg_API PUBLIC
                        ${CMAKE_CURRENT_LIST_DIR}/include/Linux/Armv7/include
                )
                target_link_libraries(CC_FFmpeg_API PUBLIC
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavcodec.so.61.19.101
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavdevice.so.61.3.100
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavfilter.so.10.4.100
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavformat.so.61.7.100
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libavutil.so.59.39.100
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libpostproc.so.58.3.100
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libswresample.so.5.3.100
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/libswscale.so.8.3.100
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/ld-linux.so.3
                        ${CMAKE_CURRENT_LIST_DIR}/bin/Linux/Armv7/ld-linux-armhf.so.3
                )
            endif()
        endif()
    endif()
endif()

#if (CMAKE_HOST_WIN32)
#    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#
#    add_custom_command(TARGET CC_FFmpeg_API POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy_if_different
#            $<TARGET_FILE:CC_FFmpeg_API>
#            ${CMAKE_BINARY_DIR}/bin
#            COMMENT "Copying CC_FFmpeg_API library to bin directory"
#    )
#endif ()

