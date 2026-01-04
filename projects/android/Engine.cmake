# Android Engine Standalone Build
# 用于 CI 中独立构建 Android 引擎库

cmake_minimum_required(VERSION 3.22.1)
project(PrismaEngineAndroid CXX)

# ========== 设置 C++ 标准 ==========
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========== Android 平台配置 ==========
set(ANDROID TRUE)

# ========== 引擎选项配置 ==========

# 禁用 Editor
set(PRISMA_BUILD_EDITOR OFF CACHE BOOL "Build Editor application" FORCE)

# 启用 Vulkan 渲染
set(PRISMA_ENABLE_RENDER_VULKAN ON CACHE BOOL "Enable Vulkan render device" FORCE)

# 禁用 DirectX12
set(PRISMA_ENABLE_RENDER_DX12 OFF CACHE BOOL "Enable DirectX12 render device" FORCE)

# 禁用 OpenGL
set(PRISMA_ENABLE_RENDER_OPENGL OFF CACHE BOOL "Enable OpenGL render device" FORCE)

# 禁用 SDL3
set(PRISMA_ENABLE_AUDIO_SDL3 OFF CACHE BOOL "Enable SDL3 audio device" FORCE)

# 使用 GLM
set(PRISMA_USE_DIRECTXMATH OFF CACHE BOOL "Use DirectXMath" FORCE)
set(PRISMA_FORCE_GLM ON CACHE BOOL "Force use GLM" FORCE)

# 启用 FetchContent
set(PRISMA_USE_FETCHCONTENT ON CACHE BOOL "Use FetchContent" FORCE)

# 启用 OpenAL 音频
set(PRISMA_ENABLE_AUDIO_OPENAL ON CACHE BOOL "Enable OpenAL audio device" FORCE)

# ========== 获取引擎根目录 ==========
get_filename_component(PRISMA_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# ========== 包含引擎 CMake 模块 ==========
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PRISMA_ENGINE_ROOT}/cmake")

# 包含设备选项配置
include(${PRISMA_ENGINE_ROOT}/cmake/DeviceOptions.cmake OPTIONAL)

# 包含 FetchContent 依赖管理
include(${PRISMA_ENGINE_ROOT}/cmake/FetchThirdPartyDeps.cmake)

# ========== 添加引擎子目录 ==========
set(ENGINE_SOURCE_DIR ${PRISMA_ENGINE_ROOT}/src/engine)
set(ENGINE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/engine)

message(STATUS "Engine source: ${ENGINE_SOURCE_DIR}")
message(STATUS "Engine binary: ${ENGINE_BINARY_DIR}")

add_subdirectory(${ENGINE_SOURCE_DIR} ${ENGINE_BINARY_DIR})

# ========== 安装规则 ==========
install(TARGETS Engine
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
