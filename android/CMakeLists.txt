# Android构建配置
cmake_minimum_required(VERSION 3.31)

project(PrismaEngineAndroid VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 输出构建信息
message(STATUS "Building PrismaEngine for Android")
message(STATUS "Android ABI: ${ANDROID_ABI}")
message(STATUS "Android Platform: ${ANDROID_PLATFORM}")
message(STATUS "Android STL: ${ANDROID_STL}")

# 包含Android工具链代理
include(${CMAKE_SOURCE_DIR}/cmake/AndroidToolchainProxy.cmake)

# 添加Engine子目录
add_subdirectory(src/engine/android engine)

# 创建聚合目标
add_custom_target(PrismaEngine
    DEPENDS engine
)