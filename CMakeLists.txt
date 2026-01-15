cmake_minimum_required(VERSION 3.31)

set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES "")

# ========== 项目声明 ==========

project(PRISMA VERSION 1.0.0 LANGUAGES CXX)

# ========== 注释: 代理配置 ==========
# 本地开发时如果需要代理，请通过环境变量 HTTP_PROXY 和 HTTPS_PROXY 设置
# 不要在 CMakeLists.txt 中硬编码代理地址，这会导致 CI 环境构建失败
#set(ENV{HTTP_PROXY} "http://127.0.0.1:7897")
#set(ENV{HTTPS_PROXY} "http://127.0.0.1:7897")

# ========== 包含配置模块 ==========

# 编译器和编译选项配置
include(cmake/CompilerOptions.cmake)

# 平台检测和配置
include(cmake/PlatformConfig.cmake)

# 输出目录配置
include(cmake/OutputDirectories.cmake)

# 设备选项配置 (需要在 FetchContent 之前)
include(cmake/DeviceOptions.cmake)

# 超分辨率选项配置 (需要在 FetchContent 之前)
include(cmake/UpscalerOptions.cmake)

# ========== 依赖管理配置 ==========

option(PRISMA_USE_FETCHCONTENT "使用 CMake FetchContent 管理依赖" ON)

if(PRISMA_USE_FETCHCONTENT)
    include(cmake/FetchThirdPartyDeps.cmake)
else()
    message(STATUS "使用系统/已安装的库")
endif()

# ========== 添加子模块 ==========

message(STATUS "")
message(STATUS "=== Adding Subdirectories ===")

# 核心模块
add_subdirectory(src/engine)
add_subdirectory(src/runtime)
add_subdirectory(src/game)

# 条件添加 Editor
if(PRISMA_BUILD_EDITOR)
    add_subdirectory(src/editor)
else()
    message(STATUS "Skipping Editor (PRISMA_BUILD_EDITOR=OFF)")
endif()

message(STATUS "==============================")
message(STATUS "")

# ========== 安装配置 ==========

#include(cmake/InstallConfig.cmake)

# ========== 项目目标和资源复制 ==========

# 此模块必须在所有子目录添加后调用
# include(cmake/ProjectTargets.cmake)

# ========== 打包配置 ==========

# include(cmake/PackagingConfig.cmake)
