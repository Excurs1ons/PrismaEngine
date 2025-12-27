#pragma once

/// @file EngineConfig.h
/// @brief 引擎主配置文件
/// 包含所有子系统的编译配置

// 包含子系统的配置文件
#include "AudioBackendConfig.h"
#include "RenderBackendConfig.h"

// ========== 全局引擎配置 ==========

// 引擎名称和版本
#ifndef PRISMA_ENGINE_NAME
    #define PRISMA_ENGINE_NAME "Prisma Engine"
#endif

#ifndef PRISMA_ENGINE_VERSION_MAJOR
    #define PRISMA_ENGINE_VERSION_MAJOR 0
#endif

#ifndef PRISMA_ENGINE_VERSION_MINOR
    #define PRISMA_ENGINE_VERSION_MINOR 1
#endif

#ifndef PRISMA_ENGINE_VERSION_PATCH
    #define PRISMA_ENGINE_VERSION_PATCH 0
#endif

// 引擎构建类型
#ifndef PRISMA_BUILD_TYPE
    #ifdef _DEBUG
        #define PRISMA_BUILD_TYPE "Debug"
    #else
        #define PRISMA_BUILD_TYPE "Release"
    #endif
#endif

// 引擎配置字符串
#define PRISMA_ENGINE_CONFIG_STRING \
    PRISMA_ENGINE_NAME " v" \
    PRISMA_STRINGIZE(PRISMA_ENGINE_VERSION_MAJOR) "." \
    PRISMA_STRINGIZE(PRISMA_ENGINE_VERSION_MINOR) "." \
    PRISMA_STRINGIZE(PRISMA_ENGINE_VERSION_PATCH) \
    " (" PRISMA_BUILD_TYPE ")"

// ========== 日志系统配置 ==========

#ifndef PRISMA_ENABLE_LOGGING
    #define PRISMA_ENABLE_LOGGING 1
#endif

#ifndef PRISMA_LOG_LEVEL
    #ifdef _DEBUG
        #define PRISMA_LOG_LEVEL 4  // Debug
    #else
        #define PRISMA_LOG_LEVEL 3  // Info
    #endif
#endif

// 日志级别定义
#define PRISMA_LOG_LEVEL_TRACE 5
#define PRISMA_LOG_LEVEL_DEBUG 4
#define PRISMA_LOG_LEVEL_INFO  3
#define PRISMA_LOG_LEVEL_WARN  2
#define PRISMA_LOG_LEVEL_ERROR 1
#define PRISMA_LOG_LEVEL_FATAL 0

// ========== 内存管理配置 ==========

#ifndef PRISMA_ENABLE_MEMORY_TRACKING
    #ifdef _DEBUG
        #define PRISMA_ENABLE_MEMORY_TRACKING 1
    #else
        #define PRISMA_ENABLE_MEMORY_TRACKING 0
    #endif
#endif

#ifndef PRISMA_ENABLE_MEMORY_POOL
    #define PRISMA_ENABLE_MEMORY_POOL 1
#endif

#ifndef PRISMA_POOL_SIZE
    #define PRISMA_POOL_SIZE (64 * 1024 * 1024)  // 64MB
#endif

// ========== 多线程配置 ==========

#ifndef PRISMA_ENABLE_MULTITHREADING
    #define PRISMA_ENABLE_MULTITHREADING 1
#endif

#ifndef PRISMA_MAX_WORKER_THREADS
    #define PRISMA_MAX_WORKER_THREADS 8
#endif

// ========== 物理系统配置 ==========

#ifndef PRISMA_ENABLE_PHYSICS
    #define PRISMA_ENABLE_PHYSICS 1
#endif

#ifndef PRISMA_PHYSICS_BACKEND
    #define PRISMA_PHYSICS_BACKEND "Bullet"  // 可选: "PhysX", "Bullet"
#endif

// ========== 网络系统配置 ==========

#ifndef PRISMA_ENABLE_NETWORKING
    #define PRISMA_ENABLE_NETWORKING 1
#endif

#ifndef PRISMA_ENABLE_CLIENT_SERVER
    #define PRISMA_ENABLE_CLIENT_SERVER 1
#endif

#ifndef PRISMA_ENABLE_P2P
    #define PRISMA_ENABLE_P2P 0
#endif

// ========== 脚本系统配置 ==========

#ifndef PRISMA_ENABLE_SCRIPTING
    #define PRISMA_ENABLE_SCRIPTING 1
#endif

#ifndef PRISMA_SCRIPT_LANGUAGE
    #define PRISMA_SCRIPT_LANGUAGE "Lua"  // 可选: "Lua", "AngelScript", "Chakra"
#endif

// ========== 资源管理配置 ==========

#ifndef PRISMA_ENABLE_RESOURCE_HOT_RELOAD
    #ifdef _DEBUG
        #define PRISMA_ENABLE_RESOURCE_HOT_RELOAD 1
    #else
        #define PRISMA_ENABLE_RESOURCE_HOT_RELOAD 0
    #endif
#endif

#ifndef PRISMA_ENABLE_ASYNC_LOADING
    #define PRISMA_ENABLE_ASYNC_LOADING 1
#endif

#ifndef PRISMA_ENABLE_RESOURCE_COMPRESSION
    #define PRISMA_ENABLE_RESOURCE_COMPRESSION 1
#endif

// ========== UI系统配置 ==========

#ifndef PRISMA_ENABLE_IMGUI
    #define PRISMA_ENABLE_IMGUI 1
#endif

#ifndef PRISMA_ENABLE_NUKLEAR
    #define PRISMA_ENABLE_NUKLEAR 0
#endif

// ========== 调试和工具配置 ==========

#ifndef PRISMA_ENABLE_PROFILER
    #ifdef _DEBUG
        #define PRISMA_ENABLE_PROFILER 1
    #else
        #define PRISMA_ENABLE_PROFILER 0
    #endif
#endif

#ifndef PRISMA_ENABLE_BENCHMARK
    #define PRISMA_ENABLE_BENCHMARK 0
#endif

#ifndef PRISMA_ENABLE_DEBUG_DRAW
    #ifdef _DEBUG
        #define PRISMA_ENABLE_DEBUG_DRAW 1
    #else
        #define PRISMA_ENABLE_DEBUG_DRAW 0
    #endif
#endif

// ========== 实用宏 ==========

// 字符串化宏
#define PRISMA_STRINGIFY(x) #x
#define PRISMA_STRINGIZE2(x) PRISMA_STRINGIFY(x)

// 平台检测
#if defined(_WIN32) || defined(_WIN64)
    #define PRISMA_PLATFORM_WINDOWS
#elif defined(__APPLE__)
    #define PRISMA_PLATFORM_APPLE
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
        #define PRISMA_PLATFORM_IOS
    #else
        #define PRISMA_PLATFORM_MACOS
    #endif
#elif defined(__ANDROID__)
    #define PRISMA_PLATFORM_ANDROID
#elif defined(__linux__)
    #define PRISMA_PLATFORM_LINUX
#endif

// 架构检测
#if defined(_M_X64) || defined(__x86_64__)
    #define PRISMA_ARCH_X64
#elif defined(_M_IX86) || defined(__i386__)
    #define PRISMA_ARCH_X86
#elif defined(_M_ARM64) || defined(__aarch64__)
    #define PRISMA_ARCH_ARM64
#elif defined(_M_ARM) || defined(__arm__)
    #define PRISMA_ARCH_ARM
#endif

// 编译器检测
#if defined(_MSC_VER)
    #define PRISMA_COMPILER_MSVC
    #define PRISMA_COMPILER_VERSION _MSC_VER
#elif defined(__clang__)
    #define PRISMA_COMPILER_CLANG
    #define PRISMA_COMPILER_VERSION __clang_major__
#elif defined(__GNUC__)
    #define PRISMA_COMPILER_GCC
    #define PRISMA_COMPILER_VERSION __GNUC__
#endif

// 编译时断言
#if defined(__cplusplus) && __cplusplus >= 201103L
    #define PRISMA_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
    #define PRISMA_STATIC_ASSERT(condition, message)
#endif

// 废弃标记
#ifdef _MSC_VER
    #define PRISMA_DEPRECATED __declspec(deprecated)
#else
    #define PRISMA_DEPRECATED __attribute__((deprecated))
#endif

// 强制内联
#ifdef _MSC_VER
    #define PRISMA_FORCE_INLINE __forceinline
#else
    #define PRISMA_FORCE_INLINE inline __attribute__((always_inline))
#endif

// 内存对齐
#define PRISMA_ALIGN(size) __attribute__((aligned(size)))

// 导出/导入宏
// 用于控制引擎 API 的可见性
#if defined(PRISMA_ENGINE_BUILD_SHARED)
    // 构建动态库
    #if defined(_MSC_VER)
        // Windows DLL
        #ifdef ENGINE_EXPORTS
            #define PRISMA_API __declspec(dllexport)
        #else
            #define PRISMA_API __declspec(dllimport)
        #endif
    #else
        // Linux/Unix 共享库
        #define PRISMA_API __attribute__((visibility("default")))
    #endif
#elif defined(PRISMA_ENGINE_BUILD_STATIC)
    // 构建静态库
    #define PRISMA_API
#else
    // 未指定构建类型，默认为静态库
    #define PRISMA_API
#endif

// 便捷宏
#define PRISMA_LOCAL  // 用于内部函数，不导出
#if defined(_MSC_VER)
    #define PRISMA_LOCAL
#else
    #define PRISMA_LOCAL __attribute__((visibility("hidden")))
#endif

// 条件编译提示
#if !PRISMA_ENABLE_LOGGING && defined(_DEBUG)
    #warning "日志系统已禁用，调试信息将不可见"
#endif

#if !PRISMA_ENABLE_MULTITHREADING && PRISMA_ENABLE_RENDER_VULKAN
    #warning "Vulkan 后端建议启用多线程以获得最佳性能"
#endif

#if PRISMA_ENABLE_RENDER_DX12 && !defined(_WIN32)
    #error "DirectX 12 仅支持 Windows 平台"
#endif

#if PRISMA_ENABLE_RENDER_METAL && !defined(__APPLE__)
    #error "Metal 仅支持 Apple 平台"
#endif