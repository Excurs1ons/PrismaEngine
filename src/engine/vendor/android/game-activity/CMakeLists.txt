# game-activity 共享库

cmake_minimum_required(VERSION 3.22)

project(game-activity)

# 源文件
set(GAME_ACTIVITY_SOURCES
    include/game-activity/GameActivity.cpp
    include/game-activity/native_app_glue/android_native_app_glue.c
    include/game-text-input/gametextinput.cpp
)

# 头文件目录
set(GAME_ACTIVITY_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/game-activity
    ${CMAKE_CURRENT_SOURCE_DIR}/include/game-activity/native_app_glue
    ${CMAKE_CURRENT_SOURCE_DIR}/include/game-text-input
)

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

# 创建共享库
add_library(game-activity SHARED
    ${GAME_ACTIVITY_SOURCES}
)

# 添加头文件路径
target_include_directories(game-activity PUBLIC
    ${GAME_ACTIVITY_INCLUDE_DIRS}
)

# 链接预编译的静态库
target_link_libraries(game-activity PUBLIC
    ${GAME_ACTIVITY_LIB_DIR}/libgame-activity.a
)

# 链接 Android 系统库
target_link_libraries(game-activity PUBLIC
    android
    log
)

message(STATUS "game-activity configured for ${ANDROID_ABI}")
