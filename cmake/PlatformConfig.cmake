# PlatformConfig.cmake
# 平台检测和配置

# ========== 平台检测 ==========

# 检测目标平台
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PRISMA_PLATFORM_WINDOWS TRUE)
    set(PRISMA_PLATFORM_NAME "windows")

    # 检测架构
    if(CMAKE_GENERATOR_PLATFORM MATCHES "Win64" OR CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
        set(PRISMA_PLATFORM_ARCH "x64")
    elseif(CMAKE_GENERATOR_PLATFORM MATCHES "ARM64" OR CMAKE_VS_PLATFORM_NAME STREQUAL "ARM64")
        set(PRISMA_PLATFORM_ARCH "arm64")
    else()
        set(PRISMA_PLATFORM_ARCH "x86")
    endif()

elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(PRISMA_PLATFORM_ANDROID TRUE)
    set(PRISMA_PLATFORM_NAME "android")
    set(PRISMA_PLATFORM_ARCH "${ANDROID_ABI}")

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PRISMA_PLATFORM_LINUX TRUE)
    set(PRISMA_PLATFORM_NAME "linux")
    set(PRISMA_PLATFORM_ARCH "${CMAKE_SYSTEM_PROCESSOR}")

elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PRISMA_PLATFORM_APPLE TRUE)
    set(PRISMA_PLATFORM_NAME "macos")

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(PRISMA_PLATFORM_ARCH "arm64")
    else()
        set(PRISMA_PLATFORM_ARCH "x64")
    endif()

else()
    set(PRISMA_PLATFORM_NAME "${CMAKE_SYSTEM_NAME}")
    set(PRISMA_PLATFORM_ARCH "unknown")
endif()

# ========== 平台信息输出 ==========

message(STATUS "")
message(STATUS "=== Platform Information ===")
message(STATUS "Platform: ${PRISMA_PLATFORM_NAME}")
message(STATUS "Architecture: ${PRISMA_PLATFORM_ARCH}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

if(PRISMA_PLATFORM_WINDOWS)
    message(STATUS "Generator: ${CMAKE_GENERATOR}")
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        message(STATUS "VS Platform: ${CMAKE_VS_PLATFORM_NAME}")
    endif()
elseif(PRISMA_PLATFORM_ANDROID)
    message(STATUS "Android ABI: ${ANDROID_ABI}")
    message(STATUS "Android NDK: ${ANDROID_NDK}")
    message(STATUS "Android API: ${ANDROID_NATIVE_API_LEVEL}")
endif()

message(STATUS "============================")
message(STATUS "")

# ========== Editor 构建选项 ==========

# Editor 构建选项 (默认在 Windows 上启用，其他平台禁用)
if(WIN32 AND NOT ANDROID)
    option(PRISMA_BUILD_EDITOR "Build Editor application" ON)
else()
    option(PRISMA_BUILD_EDITOR "Build Editor application" OFF)
endif()
