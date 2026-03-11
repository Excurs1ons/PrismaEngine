# DeviceOptions.cmake
# 设备编译选项配置

# ========== 剪枝版本配置 ==========
# 此分支简化引擎架构，仅支持：SDL3 + Vulkan + ImGui
# 平台限制：Windows x64

# 强制 SDL3 支持（跨平台输入和音频）
set(PRISMA_ENABLE_SDL3 ON CACHE BOOL "Enable SDL3 support" FORCE)

# 强制启用 Vulkan 作为唯一渲染后端
set(PRISMA_ENABLE_RENDER_DX12 OFF CACHE BOOL "Enable DirectX12" FORCE)
set(PRISMA_ENABLE_RENDER_VULKAN ON CACHE BOOL "Enable Vulkan" FORCE)
set(PRISMA_ENABLE_RENDER_OPENGL OFF CACHE BOOL "Enable OpenGL" FORCE)
set(PRISMA_DEFAULT_RENDER_BACKEND "Vulkan" CACHE STRING "Default render backend" FORCE)

# 强制启用 SDL3 作为唯一音频后端
set(PRISMA_ENABLE_AUDIO_SDL3 ON CACHE BOOL "Enable SDL3 audio support" FORCE)
set(PRISMA_ENABLE_AUDIO_XAUDIO2 OFF CACHE BOOL "Enable XAudio2 audio support" FORCE)

# ========== 高级渲染选项 ==========

option(PRISMA_ENABLE_RAY_TRACING "Enable Ray Tracing" OFF)
option(PRISMA_ENABLE_MESH_SHADERS "Enable Mesh Shaders" OFF)
option(PRISMA_ENABLE_VARIABLE_RATE_SHADING "Enable VRS" OFF)
option(PRISMA_ENABLE_BINDLESS_RESOURCES "Enable Bindless Resources" OFF)

# ========== 打印配置信息 ==========

message(STATUS "")
message(STATUS "=== Prisma Engine Configuration (Prune Branch) ===")
message(STATUS "  Simplified: SDL3 + Vulkan + ImGui only")
message(STATUS "  Platform: Windows x64 only")
message(STATUS "")
message(STATUS "Render Devices:")
if(PRISMA_ENABLE_RENDER_DX12)
    message(STATUS "  - DirectX12")
endif()
if(PRISMA_ENABLE_RENDER_VULKAN)
    message(STATUS "  - Vulkan (default)")
endif()
if(PRISMA_ENABLE_RENDER_OPENGL)
    message(STATUS "  - OpenGL")
endif()
message(STATUS "")
message(STATUS "Audio Devices:")
if(PRISMA_ENABLE_AUDIO_XAUDIO2)
    message(STATUS "  - XAudio2")
endif()
if(PRISMA_ENABLE_AUDIO_SDL3)
    message(STATUS) "  - SDL3 (default)")
endif()
message(STATUS "")
message(STATUS "Default Backend: ${PRISMA_DEFAULT_RENDER_BACKEND}")
message(STATUS "==============================================")
message(STATUS "")
