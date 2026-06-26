cmake_minimum_required(VERSION 4.2)
project(API)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(CC_SDK)
if(DEFINED ENV{OpenCV_DIR})
    set(OpenCV_DIR $ENV{OpenCV_DIR})
endif()
find_package(OpenCV REQUIRED)

add_executable(API src/main.cpp
        src/URL.cpp
        include/URL.h
        src/ID.cpp
        include/ID.h
        src/Analysis.cpp
        include/Analysis.h
        include/HeadBeat.h
        src/HeadBeat.cpp
        src/CameraSession.cpp
        include/CameraSession.h
        include/CameraSession.h
        src/CameraManager.cpp
        include/CameraManager.h
        src/PhotoManager.cpp
        include/PhotoManager.h
)

target_include_directories(API PUBLIC
        ${CC_API_INC}
)

target_link_libraries(API
        PRIVATE
        ${OpenCV_LIBS}
)

target_link_libraries(API PUBLIC
        CC_API
)
