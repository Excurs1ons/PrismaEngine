# PlatformConfig.cmake
# 平台检测和配置（prune分支：仅支持Windows x64）

# ========== 平台检测 ==========

# 检测目标平台
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PRISMA_PLATFORM_WINDOWS TRUE)
    set(PRISMA_PLATFORM_NAME "windows")

    # 检测架构 - 仅支持 x64
    if(CMAKE_GENERATOR_PLATFORM MATCHES "Win64" OR CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
        set(PRISMA_PLATFORM_ARCH "x64")
    else()
        message(FATAL_ERROR "Prisma Engine (prune分支) 仅支持 Windows x64 架构")
    endif()

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux support for LSP/IDE only
    set(PRISMA_PLATFORM_WINDOWS FALSE)
    set(PRISMA_PLATFORM_LINUX TRUE)
    set(PRISMA_PLATFORM_NAME "linux")
    set(PRISMA_PLATFORM_ARCH "x64")
    message(STATUS "Linux build: IDE/LSP support only (not for compilation)")
else()
    message(FATAL_ERROR "Prisma Engine (prune分支) 仅支持 Windows x64 平台")
endif()

# ========== 平台信息输出 ==========

message(STATUS "")
message(STATUS "=== Platform Information (Prune Branch) ===")
message(STATUS "Platform: ${PRISMA_PLATFORM_NAME}")
message(STATUS "Architecture: ${PRISMA_PLATFORM_ARCH}")
message(STATUS "Builds supported: Windows x64 only")
message(STATUS "=========================================")
message(STATUS "")

# ========== Editor 构建选项 ==========

# Editor 在 Windows x64 上启用
option(PRISMA_BUILD_EDITOR "Build Editor application" ON)
