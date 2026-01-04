# game-activity_static 静态库

cmake_minimum_required(VERSION 3.22)

project(game-activity_static)

# 源文件
set(GAME_ACTIVITY_SOURCES
    include/game-activity/GameActivity.cpp
    include/game-activity/native_app_glue/android_native_app_glue.c
    include/game-text-input/gametextinput.cpp
)

# 头文件根目录（用于导出 game-activity/xxx.h 形式的 include）
set(GAME_ACTIVITY_INCLUDE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 根据 ANDROID_ABI 选择静态库
if(ANDROID_ABI STREQUAL "arm64-v8a")
    set(GAME_ACTIVITY_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/android.arm64-v8a")
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(GAME_ACTIVITY_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/android.armeabi-v7a")
elseif(ANDROID_ABI STREQUAL "x86")
    set(GAME_ACTIVITY_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/android.x86")
elseif(ANDROID_ABI STREQUAL "x86_64")
    set(GAME_ACTIVITY_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/android.x86_64")
else()
    message(FATAL_ERROR "Unknown ANDROID_ABI: ${ANDROID_ABI}")
endif()

# 创建静态库（直接从源文件编译，不使用预编译库）
add_library(game-activity_static STATIC
    ${GAME_ACTIVITY_SOURCES}
)

# 导出头文件路径，支持 #include <game-activity/xxx.h> 形式
target_include_directories(game-activity_static PUBLIC
    ${GAME_ACTIVITY_INCLUDE_ROOT}
)

# 链接 Android 系统库
target_link_libraries(game-activity_static PUBLIC
    android
    log
)

message(STATUS "game-activity_static configured for ${ANDROID_ABI}")
message(STATUS "  Include path: ${GAME_ACTIVITY_INCLUDE_ROOT}")
message(STATUS "  Static lib: ${GAME_ACTIVITY_LIB_DIR}/libgame-activity_static.a")
