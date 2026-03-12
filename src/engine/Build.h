#pragma once

// Build.h - 构建配置相关的预处理器宏定义

// 检测 Debug 构建模式
#if defined(_DEBUG) || defined(DEBUG)
    #ifndef PRISMA_DEBUG
        #define PRISMA_DEBUG 1
    #endif
#else
    #ifndef PRISMA_DEBUG
        #define PRISMA_DEBUG 0
    #endif
#endif

// 检测是否启用 ImGui Debug 工具
#ifndef PRISMA_ENABLE_IMGUI_DEBUG
    #if PRISMA_DEBUG && WIN32
        #define PRISMA_ENABLE_IMGUI_DEBUG 1
    #else
        #define PRISMA_ENABLE_IMGUI_DEBUG 0
    #endif
#endif

// 平台检测 - Prune分支仅支持Windows x64
#if defined(_WIN32) || defined(_WIN64)
    #define PRISMA_PLATFORM_WINDOWS 1
    #define PRISMA_PLATFORM_ANDROID 0
    #define PRISMA_PLATFORM_LINUX 0
    #define PRISMA_PLATFORM_WEB 0
    #define PRISMA_PLATFORM_IOS 0
    #define PRISMA_PLATFORM_MACOS 0
#else
    #define PRISMA_PLATFORM_WINDOWS 0
    #define PRISMA_PLATFORM_ANDROID 0
    #define PRISMA_PLATFORM_LINUX 0
    #define PRISMA_PLATFORM_WEB 0
    #define PRISMA_PLATFORM_IOS 0
    #define PRISMA_PLATFORM_MACOS 0
#endif
