# DeviceOptions.cmake
# 设备编译选项配置

# ========== Native 模式选项 ==========

# Native 音频模式：仅使用平台 SDK 原生音频 API，不依赖第三方库
option(PRISMA_USE_NATIVE_AUDIO "Use platform native audio APIs only" ON)

# Native 输入模式：仅使用平台原生 input API
option(PRISMA_USE_NATIVE_INPUT "Use platform native input APIs only" ON)

# SDL3 支持：启用 SDL3 跨平台支持
option(PRISMA_ENABLE_SDL3 "Enable SDL3 support" ON)

# ========== 渲染后端选项 ==========

# 强制重置默认值逻辑
if(PRISMA_BUILD_EDITOR)
    set(PRISMA_ENABLE_RENDER_DX12 OFF CACHE BOOL "Enable DirectX12" FORCE)
    set(PRISMA_ENABLE_RENDER_VULKAN ON CACHE BOOL "Enable Vulkan" FORCE)
    set(PRISMA_DEFAULT_RENDER_BACKEND "Vulkan" CACHE STRING "Default render backend" FORCE)
else()
    # 默认设置
    if(WIN32)
        option(PRISMA_ENABLE_RENDER_DX12 "Enable DirectX12" ON)
        option(PRISMA_ENABLE_RENDER_VULKAN "Enable Vulkan" OFF)
        set(PRISMA_DEFAULT_RENDER_BACKEND "DirectX12" CACHE STRING "Default render backend")
    else()
        option(PRISMA_ENABLE_RENDER_DX12 "Enable DirectX12" OFF)
        option(PRISMA_ENABLE_RENDER_VULKAN "Enable Vulkan" ON)
        set(PRISMA_DEFAULT_RENDER_BACKEND "Vulkan" CACHE STRING "Default render backend")
    endif()
endif()

option(PRISMA_ENABLE_RENDER_OPENGL "Enable OpenGL" OFF)

# ========== 功能特性选项 ==========

option(PRISMA_ENABLE_RAY_TRACING "Enable Ray Tracing" OFF)
option(PRISMA_ENABLE_MESH_SHADERS "Enable Mesh Shaders" OFF)
option(PRISMA_ENABLE_VARIABLE_RATE_SHADING "Enable VRS" OFF)
option(PRISMA_ENABLE_BINDLESS_RESOURCES "Enable Bindless Resources" OFF)

# WebAssembly 平台强制配置
if(EMSCRIPTEN)
    set(PRISMA_USE_NATIVE_AUDIO OFF CACHE BOOL "Use platform native audio APIs only" FORCE)
    set(PRISMA_USE_NATIVE_INPUT OFF CACHE BOOL "Use platform native input APIs only" FORCE)
    set(PRISMA_BUILD_EDITOR OFF CACHE BOOL "Build Editor application" FORCE)
    set(PRISMA_BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
    set(PRISMA_RUNTIME_DYNAMIC_LOAD OFF CACHE BOOL "Runtime dynamic load for web" FORCE)
    set(PRISMA_ENABLE_RENDER_DX12 OFF CACHE BOOL "Enable DirectX12" FORCE)
    set(PRISMA_ENABLE_RENDER_VULKAN OFF CACHE BOOL "Enable Vulkan" FORCE)
    set(PRISMA_ENABLE_RENDER_OPENGL ON CACHE BOOL "Enable OpenGL" FORCE)
    set(PRISMA_DEFAULT_RENDER_BACKEND "OpenGL" CACHE STRING "Default render backend" FORCE)
endif()

# ========== 打印配置信息 ==========

message(STATUS "")
message(STATUS "=== Prisma Engine Configuration ===")
message(STATUS "Native Audio: ${PRISMA_USE_NATIVE_AUDIO}")
message(STATUS "Native Input: ${PRISMA_USE_NATIVE_INPUT}")
message(STATUS "")
message(STATUS "Render Devices:")
if(PRISMA_ENABLE_RENDER_DX12)
    message(STATUS "  - DirectX12")
endif()
if(PRISMA_ENABLE_RENDER_VULKAN)
    message(STATUS "  - Vulkan")
endif()
if(PRISMA_ENABLE_RENDER_OPENGL)
    message(STATUS "  - OpenGL")
endif()
message(STATUS "")
message(STATUS "Default Backend: ${PRISMA_DEFAULT_RENDER_BACKEND}")
message(STATUS "=====================================")
message(STATUS "")
