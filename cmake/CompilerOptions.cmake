# CompilerOptions.cmake
# 编译器和编译选项配置

# ========== C++ 标准 ==========

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========== 生成器特定配置 ==========

# Visual Studio 生成器配置
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    # 设置 Windows SDK 版本以确保使用最新版本
    set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "Windows SDK Version" FORCE)
    # 确保使用最新的 Windows SDK
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "latest" CACHE STRING "WindowsTargetPlatformVersion" FORCE)
endif()

# Ninja 生成器配置
if(CMAKE_GENERATOR MATCHES "Ninja")
    set(CMAKE_NINJA_GENERATOR_PLATFORM "x64")
    set(CMAKE_NINJA_GENERATOR_TOOLSET "host=x64")
endif()

# ========== 平台特定编译定义 ==========

# Windows 平台定义
if(WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
    add_compile_options(/DNOMINMAX /DWIN32_LEAN_AND_MEAN)
endif()

# ========== 编译选项 ==========

# Release 构建优化选项
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # MSVC 编译器选项
    add_compile_options(/W4)
    # 禁用一些常见但无用的警告
    add_compile_options(/wd4996)  # 禁用不安全函数警告
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC/Clang 编译器选项
    add_compile_options(-Wall -Wextra)
endif()

# ========== Debug 构建选项 ==========

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Debug 模式下的额外选项
    if(MSVC)
        add_compile_options(/Od)
    else()
        add_compile_options(-O0 -g)
    endif()
endif()
