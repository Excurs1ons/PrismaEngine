# DeviceOptions.cmake
# 设备编译选项配置

# ========== Native 模式选项 ==========

# Native 音频模式：仅使用平台 SDK 原生音频 API，不依赖第三方库
option(PRISMA_USE_NATIVE_AUDIO "Use platform native audio APIs only (no third-party audio dependencies)" OFF)

# Native 输入模式：仅使用平台 SDK 原生输入 API，不依赖第三方库
option(PRISMA_USE_NATIVE_INPUT "Use platform native input APIs only (no third-party input dependencies)" OFF)

# ========== 平台默认配置 ==========

# ========== 音频设备默认配置 ==========

if(PRISMA_USE_NATIVE_AUDIO)
    # Native 音频模式：使用平台 SDK 原生音频 API
    if(WIN32)
        set(PRISMA_ENABLE_AUDIO_XAUDIO2_DEFAULT ON)
        set(PRISMA_ENABLE_AUDIO_AAUDIO_DEFAULT OFF)
        set(PRISMA_ENABLE_AUDIO_COREAUDIO_DEFAULT OFF)
        set(PRISMA_ENABLE_AUDIO_ALSA_DEFAULT OFF)
        set(PRISMA_ENABLE_AUDIO_SDL3_DEFAULT OFF)
    elseif(ANDROID)
        set(PRISMA_ENABLE_AUDIO_AAUDIO_DEFAULT ON)
        set(PRISMA_ENABLE_AUDIO_XAUDIO2_DEFAULT OFF)
        set(PRISMA_ENABLE_AUDIO_COREAUDIO_DEFAULT OFF)
        set(PRISMA_ENABLE_AUDIO_ALSA_DEFAULT OFF)
        set(PRISMA_ENABLE_AUDIO_SDL3_DEFAULT OFF)
    elseif(APPLE)
        set(PRISMA_ENABLE_AUDIO_COREAUDIO_DEFAULT ON)
        set(PRISMA_ENABLE_AUDIO_XAUDIO2_DEFAULT OFF)
        set(PRISMA_ENABLE_AUDIO_AAUDIO_DEFAULT OFF)
        set(PRISMA_ENABLE_AUDIO_ALSA_DEFAULT OFF)
        set(PRISMA_ENABLE_AUDIO_SDL3_DEFAULT OFF)
    else()
        # Linux Native: ALSA
        set(PRISMA_ENABLE_AUDIO_ALSA_DEFAULT ON)
        set(PRISMA_ENABLE_AUDIO_XAUDIO2_DEFAULT OFF)
        set(PRISMA_ENABLE_AUDIO_AAUDIO_DEFAULT OFF)
        set(PRISMA_ENABLE_AUDIO_COREAUDIO_DEFAULT OFF)
        set(PRISMA_ENABLE_AUDIO_SDL3_DEFAULT OFF)
    endif()
else()
    # 跨平台模式：使用 SDL3
    set(PRISMA_ENABLE_AUDIO_SDL3_DEFAULT ON)
    set(PRISMA_ENABLE_AUDIO_XAUDIO2_DEFAULT OFF)
    set(PRISMA_ENABLE_AUDIO_AAUDIO_DEFAULT OFF)
    set(PRISMA_ENABLE_AUDIO_COREAUDIO_DEFAULT OFF)
    set(PRISMA_ENABLE_AUDIO_ALSA_DEFAULT OFF)
endif()

# ========== 输入设备默认配置 ==========

if(PRISMA_USE_NATIVE_INPUT)
    # Native 输入模式：使用平台 SDK 原生输入 API
    if(WIN32)
        set(PRISMA_ENABLE_INPUT_RAWINPUT_DEFAULT ON)
        set(PRISMA_ENABLE_INPUT_WIN32_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_XINPUT_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_GAMEACTIVITY_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_COCOA_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_EVDEV_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_SDL3_DEFAULT OFF)
    elseif(ANDROID)
        set(PRISMA_ENABLE_INPUT_GAMEACTIVITY_DEFAULT ON)
        set(PRISMA_ENABLE_INPUT_RAWINPUT_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_WIN32_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_XINPUT_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_COCOA_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_EVDEV_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_SDL3_DEFAULT OFF)
    elseif(APPLE)
        set(PRISMA_ENABLE_INPUT_COCOA_DEFAULT ON)
        set(PRISMA_ENABLE_INPUT_RAWINPUT_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_WIN32_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_XINPUT_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_GAMEACTIVITY_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_EVDEV_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_SDL3_DEFAULT OFF)
    else()
        # Linux Native: evdev
        set(PRISMA_ENABLE_INPUT_EVDEV_DEFAULT ON)
        set(PRISMA_ENABLE_INPUT_RAWINPUT_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_WIN32_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_XINPUT_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_GAMEACTIVITY_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_COCOA_DEFAULT OFF)
        set(PRISMA_ENABLE_INPUT_SDL3_DEFAULT OFF)
    endif()
else()
    # 兼容模式：使用跨平台第三方库 (默认不启用任何输入，由子项目配置)
    set(PRISMA_ENABLE_INPUT_RAWINPUT_DEFAULT OFF)
    set(PRISMA_ENABLE_INPUT_WIN32_DEFAULT OFF)
    set(PRISMA_ENABLE_INPUT_XINPUT_DEFAULT OFF)
    set(PRISMA_ENABLE_INPUT_GAMEACTIVITY_DEFAULT OFF)
    set(PRISMA_ENABLE_INPUT_COCOA_DEFAULT OFF)
    set(PRISMA_ENABLE_INPUT_EVDEV_DEFAULT OFF)
    set(PRISMA_ENABLE_INPUT_SDL3_DEFAULT OFF)
endif()

# ========== 渲染设备默认配置 (不受 Native 模式影响) ==========

if(WIN32)
    set(PRISMA_ENABLE_RENDER_DX12_DEFAULT ON)
    set(PRISMA_ENABLE_RENDER_VULKAN_DEFAULT OFF)
    set(PRISMA_ENABLE_RENDER_OPENGL_DEFAULT OFF)
    set(PRISMA_ENABLE_RENDER_METAL_DEFAULT OFF)
elseif(ANDROID)
    set(PRISMA_ENABLE_RENDER_VULKAN_DEFAULT ON)
    set(PRISMA_ENABLE_RENDER_DX12_DEFAULT OFF)
    set(PRISMA_ENABLE_RENDER_OPENGL_DEFAULT OFF)
    set(PRISMA_ENABLE_RENDER_METAL_DEFAULT OFF)
elseif(APPLE)
    set(PRISMA_ENABLE_RENDER_METAL_DEFAULT ON)
    set(PRISMA_ENABLE_RENDER_DX12_DEFAULT OFF)
    set(PRISMA_ENABLE_RENDER_VULKAN_DEFAULT OFF)
    set(PRISMA_ENABLE_RENDER_OPENGL_DEFAULT OFF)
else()
    # Linux/其他平台默认使用 Vulkan
    set(PRISMA_ENABLE_RENDER_VULKAN_DEFAULT ON)
    set(PRISMA_ENABLE_RENDER_DX12_DEFAULT OFF)
    set(PRISMA_ENABLE_RENDER_OPENGL_DEFAULT OFF)
    set(PRISMA_ENABLE_RENDER_METAL_DEFAULT OFF)
endif()

# ========== 音频设备选项 ==========

# Windows 原生
option(PRISMA_ENABLE_AUDIO_XAUDIO2 "Enable XAudio2 (Windows native)" ${PRISMA_ENABLE_AUDIO_XAUDIO2_DEFAULT})

# Android 原生
option(PRISMA_ENABLE_AUDIO_AAUDIO "Enable AAudio (Android native, API 26+)" ${PRISMA_ENABLE_AUDIO_AAUDIO_DEFAULT})

# Apple 原生
option(PRISMA_ENABLE_AUDIO_COREAUDIO "Enable CoreAudio (Apple native)" ${PRISMA_ENABLE_AUDIO_COREAUDIO_DEFAULT})

# Linux 原生
option(PRISMA_ENABLE_AUDIO_ALSA "Enable ALSA (Linux native)" ${PRISMA_ENABLE_AUDIO_ALSA_DEFAULT})
option(PRISMA_ENABLE_AUDIO_PULSEAUDIO "Enable PulseAudio (Linux native)" OFF)

# 跨平台
option(PRISMA_ENABLE_AUDIO_SDL3 "Enable SDL3 Audio (cross-platform)" ${PRISMA_ENABLE_AUDIO_SDL3_DEFAULT})

# ========== 输入设备选项 ==========

# Windows 原生
option(PRISMA_ENABLE_INPUT_RAWINPUT "Enable RawInput (Windows native)" ${PRISMA_ENABLE_INPUT_RAWINPUT_DEFAULT})
option(PRISMA_ENABLE_INPUT_WIN32 "Enable Win32 Input (Windows native)" ${PRISMA_ENABLE_INPUT_WIN32_DEFAULT})
option(PRISMA_ENABLE_INPUT_XINPUT "Enable XInput (Windows gamepad)" ${PRISMA_ENABLE_INPUT_XINPUT_DEFAULT})

# Android 原生
option(PRISMA_ENABLE_INPUT_GAMEACTIVITY "Enable GameActivity (Android native)" ${PRISMA_ENABLE_INPUT_GAMEACTIVITY_DEFAULT})

# Apple 原生
option(PRISMA_ENABLE_INPUT_COCOA "Enable Cocoa Input (Apple native)" ${PRISMA_ENABLE_INPUT_COCOA_DEFAULT})

# Linux 原生
option(PRISMA_ENABLE_INPUT_EVDEV "Enable evdev (Linux native)" ${PRISMA_ENABLE_INPUT_EVDEV_DEFAULT})
option(PRISMA_ENABLE_INPUT_LIBINPUT "Enable libinput (Linux native)" OFF)

# 跨平台第三方库
option(PRISMA_ENABLE_INPUT_SDL3 "Enable SDL3 Input (cross-platform)" ${PRISMA_ENABLE_INPUT_SDL3_DEFAULT})

# ========== 渲染设备选项 ==========

# Windows 原生
option(PRISMA_ENABLE_RENDER_DX12 "Enable DirectX12 (Windows native)" ${PRISMA_ENABLE_RENDER_DX12_DEFAULT})
option(PRISMA_ENABLE_RENDER_D3D11 "Enable DirectX11 (Windows native)" OFF)

# Android 原生 (Vulkan 在 GPU 驱动层)
option(PRISMA_ENABLE_RENDER_VULKAN "Enable Vulkan (cross-platform, Android native)" ${PRISMA_ENABLE_RENDER_VULKAN_DEFAULT})

# Apple 原生
option(PRISMA_ENABLE_RENDER_METAL "Enable Metal (Apple native)" ${PRISMA_ENABLE_RENDER_METAL_DEFAULT})

# Linux 原生
option(PRISMA_ENABLE_RENDER_OPENGL "Enable OpenGL (Linux native)" ${PRISMA_ENABLE_RENDER_OPENGL_DEFAULT})
option(PRISMA_ENABLE_RENDER_DRM "Enable DRM/KMS (Linux native, headless)" OFF)

# 跨平台
option(PRISMA_ENABLE_RENDER_WEBGPU "Enable WebGPU (Web)" OFF)

# ========== 数学库选项 ==========

option(PRISMA_USE_DIRECTXMATH "Use DirectXMath on Windows (Windows only)" OFF)
option(PRISMA_FORCE_GLM "Force use GLM on all platforms" OFF)

# ========== 功能选项 ==========

option(PRISMA_ENABLE_IMGUI_DEBUG "Enable ImGui debug UI in Debug builds" ON)

if(PRISMA_ENABLE_IMGUI_DEBUG)
    add_definitions(-DPRISMA_ENABLE_IMGUI_DEBUG=1)
endif()

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

# Windows 平台检查
if(NOT WIN32)
    if(PRISMA_ENABLE_AUDIO_XAUDIO2)
        message(WARNING "XAudio2 is only supported on Windows. Disabling...")
        set(PRISMA_ENABLE_AUDIO_XAUDIO2 OFF CACHE BOOL "" FORCE)
    endif()
    if(PRISMA_ENABLE_INPUT_RAWINPUT OR PRISMA_ENABLE_INPUT_WIN32 OR PRISMA_ENABLE_INPUT_XINPUT)
        message(WARNING "Windows input APIs are only supported on Windows. Disabling...")
        set(PRISMA_ENABLE_INPUT_RAWINPUT OFF CACHE BOOL "" FORCE)
        set(PRISMA_ENABLE_INPUT_WIN32 OFF CACHE BOOL "" FORCE)
        set(PRISMA_ENABLE_INPUT_XINPUT OFF CACHE BOOL "" FORCE)
    endif()
    if(PRISMA_ENABLE_RENDER_DX12 OR PRISMA_ENABLE_RENDER_D3D11)
        message(WARNING "DirectX is only supported on Windows. Disabling...")
        set(PRISMA_ENABLE_RENDER_DX12 OFF CACHE BOOL "" FORCE)
        set(PRISMA_ENABLE_RENDER_D3D11 OFF CACHE BOOL "" FORCE)
    endif()
endif()

# Android 平台检查
if(NOT ANDROID)
    if(PRISMA_ENABLE_AUDIO_AAUDIO)
        message(WARNING "AAudio is only supported on Android. Disabling...")
        set(PRISMA_ENABLE_AUDIO_AAUDIO OFF CACHE BOOL "" FORCE)
    endif()
    if(PRISMA_ENABLE_INPUT_GAMEACTIVITY)
        message(WARNING "GameActivity is only supported on Android. Disabling...")
        set(PRISMA_ENABLE_INPUT_GAMEACTIVITY OFF CACHE BOOL "" FORCE)
    endif()
endif()

# Apple 平台检查
if(NOT APPLE)
    if(PRISMA_ENABLE_AUDIO_COREAUDIO)
        message(WARNING "CoreAudio is only supported on Apple platforms. Disabling...")
        set(PRISMA_ENABLE_AUDIO_COREAUDIO OFF CACHE BOOL "" FORCE)
    endif()
    if(PRISMA_ENABLE_INPUT_COCOA)
        message(WARNING "Cocoa input is only supported on Apple platforms. Disabling...")
        set(PRISMA_ENABLE_INPUT_COCOA OFF CACHE BOOL "" FORCE)
    endif()
    if(PRISMA_ENABLE_RENDER_METAL)
        message(WARNING "Metal is only supported on Apple platforms. Disabling...")
        set(PRISMA_ENABLE_RENDER_METAL OFF CACHE BOOL "" FORCE)
    endif()
endif()

# Linux 平台检查
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(PRISMA_ENABLE_AUDIO_ALSA OR PRISMA_ENABLE_AUDIO_PULSEAUDIO)
        message(WARNING "Linux audio APIs are only supported on Linux. Disabling...")
        set(PRISMA_ENABLE_AUDIO_ALSA OFF CACHE BOOL "" FORCE)
        set(PRISMA_ENABLE_AUDIO_PULSEAUDIO OFF CACHE BOOL "" FORCE)
    endif()
    if(PRISMA_ENABLE_INPUT_EVDEV OR PRISMA_ENABLE_INPUT_LIBINPUT)
        message(WARNING "Linux input APIs are only supported on Linux. Disabling...")
        set(PRISMA_ENABLE_INPUT_EVDEV OFF CACHE BOOL "" FORCE)
        set(PRISMA_ENABLE_INPUT_LIBINPUT OFF CACHE BOOL "" FORCE)
    endif()
    if(PRISMA_ENABLE_RENDER_DRM)
        message(WARNING "DRM/KMS is only supported on Linux. Disabling...")
        set(PRISMA_ENABLE_RENDER_DRM OFF CACHE BOOL "" FORCE)
    endif()
endif()

# ========== 至少一个设备检查 ==========

set(HAS_AUDIO_DEVICE OFF)
set(PRISMA_AUDIO_LIST "")
if(PRISMA_ENABLE_AUDIO_XAUDIO2)
    set(HAS_AUDIO_DEVICE ON)
    list(APPEND PRISMA_AUDIO_LIST "XAudio2")
endif()
if(PRISMA_ENABLE_AUDIO_AAUDIO)
    set(HAS_AUDIO_DEVICE ON)
    list(APPEND PRISMA_AUDIO_LIST "AAudio")
endif()
if(PRISMA_ENABLE_AUDIO_COREAUDIO)
    set(HAS_AUDIO_DEVICE ON)
    list(APPEND PRISMA_AUDIO_LIST "CoreAudio")
endif()
if(PRISMA_ENABLE_AUDIO_ALSA)
    set(HAS_AUDIO_DEVICE ON)
    list(APPEND PRISMA_AUDIO_LIST "ALSA")
endif()
if(PRISMA_ENABLE_AUDIO_PULSEAUDIO)
    set(HAS_AUDIO_DEVICE ON)
    list(APPEND PRISMA_AUDIO_LIST "PulseAudio")
endif()
if(PRISMA_ENABLE_AUDIO_SDL3)
    set(HAS_AUDIO_DEVICE ON)
    list(APPEND PRISMA_AUDIO_LIST "SDL3")
endif()

if(NOT HAS_AUDIO_DEVICE)
    message(FATAL_ERROR "At least one audio device must be enabled!")
endif()

set(HAS_INPUT_DEVICE OFF)
set(PRISMA_INPUT_LIST "")
if(WIN32 AND (PRISMA_ENABLE_INPUT_RAWINPUT OR PRISMA_ENABLE_INPUT_WIN32 OR PRISMA_ENABLE_INPUT_XINPUT))
    set(HAS_INPUT_DEVICE ON)
    if(PRISMA_ENABLE_INPUT_RAWINPUT)
        list(APPEND PRISMA_INPUT_LIST "RawInput")
    endif()
    if(PRISMA_ENABLE_INPUT_WIN32)
        list(APPEND PRISMA_INPUT_LIST "Win32")
    endif()
    if(PRISMA_ENABLE_INPUT_XINPUT)
        list(APPEND PRISMA_INPUT_LIST "XInput")
    endif()
elseif(ANDROID AND PRISMA_ENABLE_INPUT_GAMEACTIVITY)
    set(HAS_INPUT_DEVICE ON)
    list(APPEND PRISMA_INPUT_LIST "GameActivity")
elseif(APPLE AND PRISMA_ENABLE_INPUT_COCOA)
    set(HAS_INPUT_DEVICE ON)
    list(APPEND PRISMA_INPUT_LIST "Cocoa")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND (PRISMA_ENABLE_INPUT_EVDEV OR PRISMA_ENABLE_INPUT_LIBINPUT))
    set(HAS_INPUT_DEVICE ON)
    if(PRISMA_ENABLE_INPUT_EVDEV)
        list(APPEND PRISMA_INPUT_LIST "evdev")
    endif()
    if(PRISMA_ENABLE_INPUT_LIBINPUT)
        list(APPEND PRISMA_INPUT_LIST "libinput")
    endif()
elseif(PRISMA_ENABLE_INPUT_SDL3)
    set(HAS_INPUT_DEVICE ON)
    list(APPEND PRISMA_INPUT_LIST "SDL3")
endif()

# 注意：输入设备不是必须的，某些平台可能不需要

set(HAS_RENDER_DEVICE OFF)
set(PRISMA_RENDER_LIST "")
if(PRISMA_ENABLE_RENDER_DX12)
    set(HAS_RENDER_DEVICE ON)
    list(APPEND PRISMA_RENDER_LIST "DirectX12")
endif()
if(PRISMA_ENABLE_RENDER_D3D11)
    set(HAS_RENDER_DEVICE ON)
    list(APPEND PRISMA_RENDER_LIST "DirectX11")
endif()
if(PRISMA_ENABLE_RENDER_VULKAN)
    set(HAS_RENDER_DEVICE ON)
    list(APPEND PRISMA_RENDER_LIST "Vulkan")
endif()
if(PRISMA_ENABLE_RENDER_METAL)
    set(HAS_RENDER_DEVICE ON)
    list(APPEND PRISMA_RENDER_LIST "Metal")
endif()
if(PRISMA_ENABLE_RENDER_OPENGL)
    set(HAS_RENDER_DEVICE ON)
    list(APPEND PRISMA_RENDER_LIST "OpenGL")
endif()
if(PRISMA_ENABLE_RENDER_DRM)
    set(HAS_RENDER_DEVICE ON)
    list(APPEND PRISMA_RENDER_LIST "DRM/KMS")
endif()
if(PRISMA_ENABLE_RENDER_WEBGPU)
    set(HAS_RENDER_DEVICE ON)
    list(APPEND PRISMA_RENDER_LIST "WebGPU")
endif()

if(NOT HAS_RENDER_DEVICE)
    message(FATAL_ERROR "At least one render device must be enabled!")
endif()

# ========== 设置预处理器定义 ==========

# Native 模式标记
if(PRISMA_USE_NATIVE_AUDIO)
    add_definitions(-DPRISMA_USE_NATIVE_AUDIO=1)
endif()
if(PRISMA_USE_NATIVE_INPUT)
    add_definitions(-DPRISMA_USE_NATIVE_INPUT=1)
endif()

# 音频设备
if(PRISMA_ENABLE_AUDIO_XAUDIO2)
    add_definitions(-DPRISMA_ENABLE_AUDIO_XAUDIO2=1)
endif()
if(PRISMA_ENABLE_AUDIO_AAUDIO)
    add_definitions(-DPRISMA_ENABLE_AUDIO_AAUDIO=1)
endif()
if(PRISMA_ENABLE_AUDIO_COREAUDIO)
    add_definitions(-DPRISMA_ENABLE_AUDIO_COREAUDIO=1)
endif()
if(PRISMA_ENABLE_AUDIO_ALSA)
    add_definitions(-DPRISMA_ENABLE_AUDIO_ALSA=1)
endif()
if(PRISMA_ENABLE_AUDIO_PULSEAUDIO)
    add_definitions(-DPRISMA_ENABLE_AUDIO_PULSEAUDIO=1)
endif()
if(PRISMA_ENABLE_AUDIO_SDL3)
    add_definitions(-DPRISMA_ENABLE_AUDIO_SDL3=1)
endif()

# 输入设备
if(PRISMA_ENABLE_INPUT_RAWINPUT)
    add_definitions(-DPRISMA_ENABLE_INPUT_RAWINPUT=1)
endif()
if(PRISMA_ENABLE_INPUT_WIN32)
    add_definitions(-DPRISMA_ENABLE_INPUT_WIN32=1)
endif()
if(PRISMA_ENABLE_INPUT_XINPUT)
    add_definitions(-DPRISMA_ENABLE_INPUT_XINPUT=1)
endif()
if(PRISMA_ENABLE_INPUT_GAMEACTIVITY)
    add_definitions(-DPRISMA_ENABLE_INPUT_GAMEACTIVITY=1)
endif()
if(PRISMA_ENABLE_INPUT_COCOA)
    add_definitions(-DPRISMA_ENABLE_INPUT_COCOA=1)
endif()
if(PRISMA_ENABLE_INPUT_EVDEV)
    add_definitions(-DPRISMA_ENABLE_INPUT_EVDEV=1)
endif()
if(PRISMA_ENABLE_INPUT_LIBINPUT)
    add_definitions(-DPRISMA_ENABLE_INPUT_LIBINPUT=1)
endif()
if(PRISMA_ENABLE_INPUT_SDL3)
    add_definitions(-DPRISMA_ENABLE_INPUT_SDL3=1)
endif()

# 渲染设备
if(PRISMA_ENABLE_RENDER_DX12)
    add_definitions(-DPRISMA_ENABLE_RENDER_DX12=1)
endif()
if(PRISMA_ENABLE_RENDER_D3D11)
    add_definitions(-DPRISMA_ENABLE_RENDER_D3D11=1)
endif()
if(PRISMA_ENABLE_RENDER_VULKAN)
    add_definitions(-DPRISMA_ENABLE_RENDER_VULKAN=1)
endif()
if(PRISMA_ENABLE_RENDER_METAL)
    add_definitions(-DPRISMA_ENABLE_RENDER_METAL=1)
endif()
if(PRISMA_ENABLE_RENDER_OPENGL)
    add_definitions(-DPRISMA_ENABLE_RENDER_OPENGL=1)
endif()
if(PRISMA_ENABLE_RENDER_DRM)
    add_definitions(-DPRISMA_ENABLE_RENDER_DRM=1)
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
message(STATUS "Native Audio: ${PRISMA_USE_NATIVE_AUDIO}")
message(STATUS "Native Input: ${PRISMA_USE_NATIVE_INPUT}")

message(STATUS "")
message(STATUS "Audio Devices:")
foreach(AUDIO ${PRISMA_AUDIO_LIST})
    message(STATUS "  ${AUDIO}")
endforeach()

message(STATUS "")
message(STATUS "Input Devices:")
if(PRISMA_INPUT_LIST)
    foreach(INPUT ${PRISMA_INPUT_LIST})
        message(STATUS "  ${INPUT}")
    endforeach()
else()
    message(STATUS "  (none)")
endif()

message(STATUS "")
message(STATUS "Render Devices:")
foreach(RENDER ${PRISMA_RENDER_LIST})
    message(STATUS "  ${RENDER}")
endforeach()

message(STATUS "")
message(STATUS "Advanced Features:")
message(STATUS "  Audio 3D:            ${PRISMA_ENABLE_AUDIO_3D}")
message(STATUS "  Audio Streaming:     ${PRISMA_ENABLE_AUDIO_STREAMING}")
message(STATUS "  Audio Effects:       ${PRISMA_ENABLE_AUDIO_EFFECTS}")
message(STATUS "  Ray Tracing:         ${PRISMA_ENABLE_RAYTRACING}")
message(STATUS "  Mesh Shaders:        ${PRISMA_ENABLE_MESH_SHADERS}")
message(STATUS "  Variable Rate Shading: ${PRISMA_ENABLE_VARIABLE_RATE_SHADING}")
message(STATUS "  Bindless Resources:  ${PRISMA_ENABLE_BINDLESS_RESOURCES}")
message(STATUS "=====================================")
message(STATUS "")
