# PlatformConfig.cmake
# 平台检测和配置（prune分支：优先支持 SDL3 + Vulkan 可适配平台）

# ========== 平台检测 ==========

set(PRISMA_PLATFORM_WINDOWS FALSE)
set(PRISMA_PLATFORM_LINUX FALSE)
set(PRISMA_PLATFORM_MACOS FALSE)
set(PRISMA_PLATFORM_ANDROID FALSE)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PRISMA_PLATFORM_WINDOWS TRUE)
    set(PRISMA_PLATFORM_NAME "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PRISMA_PLATFORM_LINUX TRUE)
    set(PRISMA_PLATFORM_NAME "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PRISMA_PLATFORM_MACOS TRUE)
    set(PRISMA_PLATFORM_NAME "macos")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(PRISMA_PLATFORM_ANDROID TRUE)
    set(PRISMA_PLATFORM_NAME "android")
else()
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" PRISMA_PLATFORM_NAME)
endif()

# ========== 架构检测 ==========

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _prisma_processor)
if(_prisma_processor MATCHES "^(x86_64|amd64)$")
    set(PRISMA_PLATFORM_ARCH "x64")
elseif(_prisma_processor MATCHES "^(aarch64|arm64)$")
    set(PRISMA_PLATFORM_ARCH "arm64")
elseif(_prisma_processor MATCHES "^(armv7|armv7l)$")
    set(PRISMA_PLATFORM_ARCH "armv7")
else()
    set(PRISMA_PLATFORM_ARCH "${_prisma_processor}")
endif()

# ========== 平台信息输出 ==========

message(STATUS "")
message(STATUS "=== Platform Information (Prune Branch) ===")
message(STATUS "Platform: ${PRISMA_PLATFORM_NAME}")
message(STATUS "Architecture: ${PRISMA_PLATFORM_ARCH}")
message(STATUS "Build policy: SDL3 + Vulkan capable platforms are allowed")
message(STATUS "=========================================")
message(STATUS "")

# ========== Editor 构建选项 ==========

option(PRISMA_BUILD_EDITOR "Build Editor application" OFF)
