# DeviceOptions.cmake
# 设备编译选项配置

# ========== Native 模式选项 ==========

# Native 音频模式：仅使用平台 SDK 原生音频 API，不依赖第三方库
option(PRISMA_USE_NATIVE_AUDIO "Use platform native audio APIs only" ON)

# Native 输入模式：仅使用平台原生输入 API
option(PRISMA_USE_NATIVE_INPUT "Use platform native input APIs only" ON)

# SDL3 支持：启用 SDL3 跨平台支持（窗口、输入、音频）
option(PRISMA_ENABLE_SDL3 "Enable SDL3 support" ON)

# ========== 渲染后端选项 ==========

# 默认设置
set(PRISMA_ENABLE_RENDER_DX12_DEFAULT OFF)
set(PRISMA_ENABLE_RENDER_VULKAN_DEFAULT ON)

# Windows 平台特殊处理
if(WIN32)
    # 如果不是构建编辑器，默认可以用 DX12
    if(NOT PRISMA_BUILD_EDITOR)
        set(PRISMA_ENABLE_RENDER_DX12_DEFAULT ON)
        set(PRISMA_ENABLE_RENDER_VULKAN_DEFAULT OFF)
    endif()
endif()

option(PRISMA_ENABLE_RENDER_DX12 "Enable DirectX12" ${PRISMA_ENABLE_RENDER_DX12_DEFAULT})
option(PRISMA_ENABLE_RENDER_VULKAN "Enable Vulkan" ${PRISMA_ENABLE_RENDER_VULKAN_DEFAULT})
option(PRISMA_ENABLE_RENDER_OPENGL "Enable OpenGL" OFF)

# 强制设置默认后端
if(PRISMA_BUILD_EDITOR OR PRISMA_ENABLE_RENDER_VULKAN)
    set(PRISMA_DEFAULT_RENDER_BACKEND "Vulkan" CACHE STRING "Default render backend" FORCE)
else()
    set(PRISMA_DEFAULT_RENDER_BACKEND "DirectX12" CACHE STRING "Default render backend" FORCE)
endif()

# ========== 功能特性选项 ==========

option(PRISMA_ENABLE_RAY_TRACING "Enable Ray Tracing" OFF)
option(PRISMA_ENABLE_MESH_SHADERS "Enable Mesh Shaders" OFF)
option(PRISMA_ENABLE_VARIABLE_RATE_SHADING "Enable VRS" OFF)
option(PRISMA_ENABLE_BINDLESS_RESOURCES "Enable Bindless Resources" OFF)

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
