# DeviceOptions.cmake
# 设备编译选项配置

# ========== 平台默认配置 ==========

# Windows 平台默认使用 DirectX12 + XAudio2
if(WIN32)
    set(PRISMA_ENABLE_RENDER_DX12_DEFAULT ON)
    set(PRISMA_ENABLE_AUDIO_XAUDIO2_DEFAULT ON)
    set(PRISMA_ENABLE_RENDER_VULKAN_DEFAULT OFF)
    set(PRISMA_ENABLE_AUDIO_OPENAL_DEFAULT OFF)
elseif(ANDROID)
    # Android 平台默认使用 Vulkan + OpenAL
    set(PRISMA_ENABLE_RENDER_VULKAN_DEFAULT ON)
    set(PRISMA_ENABLE_AUDIO_OPENAL_DEFAULT ON)
    set(PRISMA_ENABLE_RENDER_DX12_DEFAULT OFF)
    set(PRISMA_ENABLE_AUDIO_XAUDIO2_DEFAULT OFF)
elseif(APPLE)
    # macOS/iOS 平台默认使用 Metal + OpenAL
    set(PRISMA_ENABLE_RENDER_METAL_DEFAULT ON)
    set(PRISMA_ENABLE_AUDIO_OPENAL_DEFAULT ON)
    set(PRISMA_ENABLE_RENDER_VULKAN_DEFAULT OFF)
    set(PRISMA_ENABLE_AUDIO_XAUDIO2_DEFAULT OFF)
else()
    # Linux/其他平台默认使用 Vulkan + OpenAL
    set(PRISMA_ENABLE_RENDER_VULKAN_DEFAULT ON)
    set(PRISMA_ENABLE_AUDIO_OPENAL_DEFAULT ON)
    set(PRISMA_ENABLE_RENDER_DX12_DEFAULT OFF)
    set(PRISMA_ENABLE_AUDIO_XAUDIO2_DEFAULT OFF)
    set(PRISMA_ENABLE_RENDER_METAL_DEFAULT OFF)
endif()

# ========== 音频设备选项 ==========

# XAudio2 (Windows)
option(PRISMA_ENABLE_AUDIO_XAUDIO2 "Enable XAudio2 audio device (Windows only)" ${PRISMA_ENABLE_AUDIO_XAUDIO2_DEFAULT})

# OpenAL (跨平台)
option(PRISMA_ENABLE_AUDIO_OPENAL "Enable OpenAL audio device" ${PRISMA_ENABLE_AUDIO_OPENAL_DEFAULT})

# SDL3 Audio (跨平台)
option(PRISMA_ENABLE_AUDIO_SDL3 "Enable SDL3 audio device" OFF)

# ========== 渲染设备选项 ==========

# DirectX12 (Windows)
option(PRISMA_ENABLE_RENDER_DX12 "Enable DirectX12 render device (Windows only)" ${PRISMA_ENABLE_RENDER_DX12_DEFAULT})

# OpenGL (跨平台)
option(PRISMA_ENABLE_RENDER_OPENGL "Enable OpenGL render device" OFF)

# Vulkan (跨平台)
option(PRISMA_ENABLE_RENDER_VULKAN "Enable Vulkan render device" ${PRISMA_ENABLE_RENDER_VULKAN_DEFAULT})

# Metal (macOS/iOS)
option(PRISMA_ENABLE_RENDER_METAL "Enable Metal render device (Apple only)" ${PRISMA_ENABLE_RENDER_METAL_DEFAULT})

# WebGPU (Web)
option(PRISMA_ENABLE_RENDER_WEBGPU "Enable WebGPU render device" OFF)

# ========== 数学库选项 ==========

# 数学库选择
option(PRISMA_USE_DIRECTXMATH "Use DirectXMath on Windows (Windows only)" OFF)
option(PRISMA_FORCE_GLM "Force use GLM on all platforms" OFF)

# ========== 功能选项 ==========

# 音频功能
option(PRISMA_ENABLE_AUDIO_3D "Enable 3D audio support" ON)
option(PRISMA_ENABLE_AUDIO_STREAMING "Enable audio streaming" ON)
option(PRISMA_ENABLE_AUDIO_EFFECTS "Enable audio effects (EAX/EFX)" OFF)
option(PRISMA_ENABLE_AUDIO_HRTF "Enable HRTF audio" OFF)

# 渲染功能
option(PRISMA_ENABLE_RAYTRACING "Enable hardware ray tracing" OFF)
option(PRISMA_ENABLE_MESH_SHADERS "Enable mesh shaders" OFF)
option(PRISMA_ENABLE_VARIABLE_RATE_SHADING "Enable variable rate shading" OFF)
option(PRISMA_ENABLE_BINDLESS_RESOURCES "Enable bindless resources" OFF)

# ========== 平台检查 ==========

# 音频后端平台检查
if(NOT WIN32 AND PRISMA_ENABLE_AUDIO_XAUDIO2)
    message(WARNING "XAudio2 is only supported on Windows. Disabling...")
    set(PRISMA_ENABLE_AUDIO_XAUDIO2 OFF CACHE BOOL "Enable XAudio2 audio backend" FORCE)
endif()

# 渲染后端平台检查
if(NOT WIN32 AND PRISMA_ENABLE_RENDER_DX12)
    message(WARNING "DirectX12 is only supported on Windows. Disabling...")
    set(PRISMA_ENABLE_RENDER_DX12 OFF CACHE BOOL "Enable DirectX12 render backend" FORCE)
endif()

if(NOT APPLE AND PRISMA_ENABLE_RENDER_METAL)
    message(WARNING "Metal is only supported on Apple platforms. Disabling...")
    set(PRISMA_ENABLE_RENDER_METAL OFF CACHE BOOL "Enable Metal render backend" FORCE)
endif()

if(NOT EMSCRIPTEN AND PRISMA_ENABLE_RENDER_WEBGPU)
    message(WARNING "WebGPU is only supported on Emscripten. Disabling...")
    set(PRISMA_ENABLE_RENDER_WEBGPU OFF CACHE BOOL "Enable WebGPU render backend" FORCE)
endif()

# ========== 至少一个设备检查 ==========

set(HAS_AUDIO_DEVICE OFF)
if(PRISMA_ENABLE_AUDIO_XAUDIO2 OR PRISMA_ENABLE_AUDIO_OPENAL OR PRISMA_ENABLE_AUDIO_SDL3)
    set(HAS_AUDIO_DEVICE ON)
endif()

if(NOT HAS_AUDIO_DEVICE)
    if(PRISMA_ENABLE_AUDIO_OPENAL_DEFAULT)
        message(STATUS "没有配置音频设备，已自动选择平台默认值:OPENAL")
        set(PRISMA_ENABLE_AUDIO_OPENAL ON)
    elseif (PRISMA_ENABLE_AUDIO_XAUDIO2_DEFAULT)
        message(STATUS "没有配置音频设备，已自动选择平台默认值:XAUDIO2")
        set(PRISMA_ENABLE_AUDIO_XAUDIO2 ON)
    elseif (PRISMA_ENABLE_AUDIO_SDL3_DEFAULT)
        message(STATUS "没有配置音频设备，已自动选择平台默认值:SDL3")
        set(PRISMA_ENABLE_AUDIO_SDL3 ON)
    endif ()

endif()

set(HAS_RENDER_DEVICE OFF)
if(PRISMA_ENABLE_RENDER_DX12 OR PRISMA_ENABLE_RENDER_OPENGL OR
   PRISMA_ENABLE_RENDER_VULKAN OR PRISMA_ENABLE_RENDER_METAL OR
   PRISMA_ENABLE_RENDER_WEBGPU)
    set(HAS_RENDER_DEVICE ON)
endif()

if(NOT HAS_RENDER_DEVICE)
    message(FATAL_ERROR "At least one render device must be enabled!")
endif()

# ========== 设置预处理器定义 ==========

# 音频设备
if(PRISMA_ENABLE_AUDIO_XAUDIO2)
    add_definitions(-DPRISMA_ENABLE_AUDIO_XAUDIO2=1)
endif()
if(PRISMA_ENABLE_AUDIO_OPENAL)
    add_definitions(-DPRISMA_ENABLE_AUDIO_OPENAL=1)
endif()
if(PRISMA_ENABLE_AUDIO_SDL3)
    add_definitions(-DPRISMA_ENABLE_AUDIO_SDL3=1)
endif()

# 渲染设备
if(PRISMA_ENABLE_RENDER_DX12)
    add_definitions(-DPRISMA_ENABLE_RENDER_DX12=1)
endif()
if(PRISMA_ENABLE_RENDER_OPENGL)
    add_definitions(-DPRISMA_ENABLE_RENDER_OPENGL=1)
endif()
if(PRISMA_ENABLE_RENDER_VULKAN)
    add_definitions(-DPRISMA_ENABLE_RENDER_VULKAN=1)
endif()
if(PRISMA_ENABLE_RENDER_METAL)
    add_definitions(-DPRISMA_ENABLE_RENDER_METAL=1)
endif()
if(PRISMA_ENABLE_RENDER_WEBGPU)
    add_definitions(-DPRISMA_ENABLE_RENDER_WEBGPU=1)
endif()

# 功能选项
if(PRISMA_ENABLE_AUDIO_3D)
    add_definitions(-DPRISMA_ENABLE_AUDIO_3D=1)
endif()
if(PRISMA_ENABLE_AUDIO_STREAMING)
    add_definitions(-DPRISMA_ENABLE_AUDIO_STREAMING=1)
endif()
if(PRISMA_ENABLE_AUDIO_EFFECTS)
    add_definitions(-DPRISMA_ENABLE_AUDIO_EFFECTS=1)
endif()
if(PRISMA_ENABLE_AUDIO_HRTF)
    add_definitions(-DPRISMA_ENABLE_AUDIO_HRTF=1)
endif()

if(PRISMA_ENABLE_RAYTRACING)
    add_definitions(-DPRISMA_ENABLE_RAYTRACING=1)
endif()
if(PRISMA_ENABLE_MESH_SHADERS)
    add_definitions(-DPRISMA_ENABLE_MESH_SHADERS=1)
endif()
if(PRISMA_ENABLE_VARIABLE_RATE_SHADING)
    add_definitions(-DPRISMA_ENABLE_VARIABLE_RATE_SHADING=1)
endif()
if(PRISMA_ENABLE_BINDLESS_RESOURCES)
    add_definitions(-DPRISMA_ENABLE_BINDLESS_RESOURCES=1)
endif()

# ========== 打印配置信息 ==========

message(STATUS "")
message(STATUS "=== Prisma Engine Configuration ===")
message(STATUS "Math Library:")
if(WIN32 AND PRISMA_USE_DIRECTXMATH)
    message(STATUS "  DirectXMath: ON")
    message(STATUS "  GLM:         OFF")
else()
    message(STATUS "  DirectXMath: OFF")
    message(STATUS "  GLM:         ON")
endif()
message(STATUS "")
message(STATUS "Audio Devices:")
message(STATUS "  XAudio2: ${PRISMA_ENABLE_AUDIO_XAUDIO2}")
message(STATUS "  OpenAL:   ${PRISMA_ENABLE_AUDIO_OPENAL}")
message(STATUS "  SDL3:     ${PRISMA_ENABLE_AUDIO_SDL3}")
message(STATUS "")
message(STATUS "Render Devices:")
message(STATUS "  DirectX12: ${PRISMA_ENABLE_RENDER_DX12}")
message(STATUS "  OpenGL:    ${PRISMA_ENABLE_RENDER_OPENGL}")
message(STATUS "  Vulkan:    ${PRISMA_ENABLE_RENDER_VULKAN}")
#message(STATUS "  Metal:     ${PRISMA_ENABLE_RENDER_METAL}")
#message(STATUS "  WebGPU:    ${PRISMA_ENABLE_RENDER_WEBGPU}")
message(STATUS "")
message(STATUS "Advanced Features:")
message(STATUS "  Audio 3D:            ${PRISMA_ENABLE_AUDIO_3D}")
message(STATUS "  Audio Streaming:     ${PRISMA_ENABLE_AUDIO_STREAMING}")
message(STATUS "  Audio Effects:       ${PRISMA_ENABLE_AUDIO_EFFECTS}")
message(STATUS "  Ray Tracing:        ${PRISMA_ENABLE_RAYTRACING}")
message(STATUS "  Mesh Shaders:       ${PRISMA_ENABLE_MESH_SHADERS}")
message(STATUS "  Variable Rate Shading: ${PRISMA_ENABLE_VARIABLE_RATE_SHADING}")
message(STATUS "  Bindless Resources:  ${PRISMA_ENABLE_BINDLESS_RESOURCES}")
message(STATUS "=====================================")
message(STATUS "")