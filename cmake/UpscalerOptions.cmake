# UpscalerOptions.cmake
# 超分辨率技术选项配置 / Upscaler Technology Options Configuration

# ========== 默认配置 / Default Configuration ==========

# Windows 平台默认配置
set(PRISMA_ENABLE_UPSCALER_FSR_DEFAULT OFF)
set(PRISMA_ENABLE_UPSCALER_DLSS_DEFAULT OFF)
set(PRISMA_ENABLE_UPSCALER_TSR_DEFAULT OFF)

if(WIN32)
    # Windows: FSR + DLSS + TSR
    set(PRISMA_ENABLE_UPSCALER_FSR_DEFAULT ON)
    set(PRISMA_ENABLE_UPSCALER_DLSS_DEFAULT ON)
    set(PRISMA_ENABLE_UPSCALER_TSR_DEFAULT ON)
elseif(ANDROID)
    # Android: FSR + TSR（DLSS 不支持 / DLSS not supported）
    set(PRISMA_ENABLE_UPSCALER_FSR_DEFAULT ON)
    set(PRISMA_ENABLE_UPSCALER_TSR_DEFAULT ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux: FSR + DLSS (Vulkan) + TSR
    set(PRISMA_ENABLE_UPSCALER_FSR_DEFAULT ON)
    set(PRISMA_ENABLE_UPSCALER_DLSS_DEFAULT ON)
    set(PRISMA_ENABLE_UPSCALER_TSR_DEFAULT ON)
endif()

# ========== 超分辨率选项 / Upscaler Options ==========

option(PRISMA_ENABLE_UPSCALER_FSR "Enable AMD FidelityFX Super Resolution (FSR 3.1)" ${PRISMA_ENABLE_UPSCALER_FSR_DEFAULT})
option(PRISMA_ENABLE_UPSCALER_DLSS "Enable NVIDIA DLSS (Windows/Linux only, requires RTX GPU)" ${PRISMA_ENABLE_UPSCALER_DLSS_DEFAULT})
option(PRISMA_ENABLE_UPSCALER_TSR "Enable Temporal Super Resolution (custom implementation)" ${PRISMA_ENABLE_UPSCALER_TSR_DEFAULT})

# ========== 超分辨率功能选项 / Upscaler Feature Options ==========

option(PRISMA_ENABLE_UPSCALER_MOTION_VECTORS "Enable motion vectors generation (required for most upscalers)" ON)
option(PRISMA_ENABLE_UPSCALER_DEBUG "Enable upscaler debug visualization" OFF)

# ========== 平台检查 / Platform Checks ==========

# DLSS 平台检查 / DLSS platform check
if(ANDROID)
    if(PRISMA_ENABLE_UPSCALER_DLSS)
        message(WARNING "DLSS is not supported on Android. Disabling...")
        set(PRISMA_ENABLE_UPSCALER_DLSS OFF CACHE BOOL "" FORCE)
    endif()
endif()

# ========== 至少一个超分技术检查 / At Least One Upscaler Check ==========

set(HAS_UPSCALER OFF)
set(PRISMA_UPSCALER_LIST "")

if(PRISMA_ENABLE_UPSCALER_FSR)
    set(HAS_UPSCALER ON)
    list(APPEND PRISMA_UPSCALER_LIST "FSR")
endif()

if(PRISMA_ENABLE_UPSCALER_DLSS)
    set(HAS_UPSCALER ON)
    list(APPEND PRISMA_UPSCALER_LIST "DLSS")
endif()

if(PRISMA_ENABLE_UPSCALER_TSR)
    set(HAS_UPSCALER ON)
    list(APPEND PRISMA_UPSCALER_LIST "TSR")
endif()

if(NOT HAS_UPSCALER)
    message(STATUS "No upscaler enabled. Consider enabling at least one (FSR recommended).")
endif()

# ========== 设置预处理器定义 / Set Preprocessor Definitions ==========

if(PRISMA_ENABLE_UPSCALER_FSR)
    add_definitions(-DPRISMA_ENABLE_UPSCALER_FSR=1)
endif()

if(PRISMA_ENABLE_UPSCALER_DLSS)
    add_definitions(-DPRISMA_ENABLE_UPSCALER_DLSS=1)
endif()

if(PRISMA_ENABLE_UPSCALER_TSR)
    add_definitions(-DPRISMA_ENABLE_UPSCALER_TSR=1)
endif()

if(PRISMA_ENABLE_UPSCALER_MOTION_VECTORS)
    add_definitions(-DPRISMA_ENABLE_UPSCALER_MOTION_VECTORS=1)
endif()

if(PRISMA_ENABLE_UPSCALER_DEBUG)
    add_definitions(-DPRISMA_ENABLE_UPSCALER_DEBUG=1)
endif()

# ========== 打印配置信息 / Print Configuration ==========

message(STATUS "")
message(STATUS "=== Upscaler Configuration ===")
if(PRISMA_UPSCALER_LIST)
    foreach(UPSCALER ${PRISMA_UPSCALER_LIST})
        message(STATUS "  ${UPSCALER}")
    endforeach()
else()
    message(STATUS "  (none)")
endif()
message(STATUS "  Motion Vectors:      ${PRISMA_ENABLE_UPSCALER_MOTION_VECTORS}")
message(STATUS "  Debug Visualization: ${PRISMA_ENABLE_UPSCALER_DEBUG}")
message(STATUS "================================")
message(STATUS "")
