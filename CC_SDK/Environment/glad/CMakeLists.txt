cmake_minimum_required(VERSION 3.0...4.0)

project(glad)

set( GLAD_SHARED OFF CACHE BOOL "Build the shared glad library" )

include_directories(${PROJECT_SOURCE_DIR}/include)

if(GLAD_SHARED)
message(STATUS "build shared glad")
add_library(glad SHARED glad.c)
else()
message(STATUS "build static glad")
add_library(glad STATIC glad.c)
endif()

set_target_properties(glad PROPERTIES DEBUG_POSTFIX d)
