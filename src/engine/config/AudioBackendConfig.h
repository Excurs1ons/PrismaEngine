#pragma once

/// @file AudioBackendConfig.h
/// @brief 音频设备编译配置
/// 用于控制哪些音频设备被编译到最终版本中

// ========== 音频设备编译控制 ==========

// 默认只启用 XAudio2 (Windows 平台优化)
// 如需启用其他设备，请取消相应注释

// Windows 平台 (默认启用)
#ifndef PRISMA_ENABLE_AUDIO_XAUDIO2
    #ifdef _WIN32
        #define PRISMA_ENABLE_AUDIO_XAUDIO2 1  // Windows XAudio2 (高性能)
    #else
        #define PRISMA_ENABLE_AUDIO_XAUDIO2 0
    #endif
#endif

// 跨平台音频后端 (默认禁用)
#ifndef PRISMA_ENABLE_AUDIO_OPENAL
    #define PRISMA_ENABLE_AUDIO_OPENAL 0      // OpenAL 跨平台 3D 音频
#endif

#ifndef PRISMA_ENABLE_AUDIO_SDL3
    #define PRISMA_ENABLE_AUDIO_SDL3 0       // SDL3 Audio 简单跨平台
#endif

// 空实现 (总是可用)
#ifndef PRISMA_ENABLE_AUDIO_NULL
    #define PRISMA_ENABLE_AUDIO_NULL 1        // 静音设备，用于测试
#endif

// ========== 音频功能宏 ==========

#ifndef PRISMA_ENABLE_AUDIO_3D
    #define PRISMA_ENABLE_AUDIO_3D 1         // 3D 音频支持
#endif

#ifndef PRISMA_ENABLE_AUDIO_STREAMING
    #define PRISMA_ENABLE_AUDIO_STREAMING 1   // 流式音频支持
#endif

#ifndef PRISMA_ENABLE_AUDIO_EFFECTS
    #define PRISMA_ENABLE_AUDIO_EFFECTS 0     // 音效处理 (EAX/EFX)
#endif

#ifndef PRISMA_ENABLE_AUDIO_HRTF
    #define PRISMA_ENABLE_AUDIO_HRTF 0        // 双耳音频 (HRTF)
#endif

// ========== 平台特定优化 ==========

// Windows 特定优化
#ifdef _WIN32
    #ifndef PRISMA_AUDIO_USE_XAUDIO2_THREADS
        #define PRISMA_AUDIO_USE_XAUDIO2_THREADS 1  // XAudio2 多线程支持
    #endif
#endif

// Linux/Unix 特定优化
#ifdef __linux__
    #ifndef PRISMA_AUDIO_USE_ALSA
        #define PRISMA_AUDIO_USE_ALSA 1         // ALSA 支持 (通过 OpenAL)
    #endif
#endif

// Android 特定优化
#ifdef __ANDROID__
    #ifndef PRISMA_AUDIO_USE_OPENSLES
        #define PRISMA_AUDIO_USE_OPENSLES 1     // OpenSL ES 支持
    #endif
#endif

// ========== 编译时检查 ==========

// 确保至少有一个后端可用
#if !PRISMA_ENABLE_AUDIO_XAUDIO2 && \
    !PRISMA_ENABLE_AUDIO_OPENAL && \
    !PRISMA_ENABLE_AUDIO_SDL3 && \
    !PRISMA_ENABLE_AUDIO_NULL
    #error "至少需要启用一个音频后端"
#endif

// 确保主后端可用
#if defined(_WIN32) && !PRISMA_ENABLE_AUDIO_XAUDIO2 && !PRISMA_ENABLE_AUDIO_OPENAL && !PRISMA_ENABLE_AUDIO_SDL3
    #warning "Windows 平台未启用主要音频后端，建议启用 XAudio2 或 OpenAL"
#endif

#if defined(__APPLE__) && !PRISMA_ENABLE_AUDIO_OPENAL && !PRISMA_ENABLE_AUDIO_SDL3
    #warning "macOS 平台未启用音频后端，建议启用 OpenAL 或 SDL3"
#endif

// ========== 便利宏 ==========

// 定义启用的后端列表
#if PRISMA_ENABLE_AUDIO_XAUDIO2
    #define PRISMA_AVAILABLE_AUDIO_XAUDIO2
#endif

#if PRISMA_ENABLE_AUDIO_OPENAL
    #define PRISMA_AVAILABLE_AUDIO_OPENAL
#endif

#if PRISMA_ENABLE_AUDIO_SDL3
    #define PRISMA_AVAILABLE_AUDIO_SDL3
#endif

#if PRISMA_ENABLE_AUDIO_NULL
    #define PRISMA_AVAILABLE_AUDIO_NULL
#endif

// 条件编译宏
#if PRISMA_ENABLE_AUDIO_XAUDIO2
    #define PRISMA_IF_XAUDIO2_ENABLED(code) code
#else
    #define PRISMA_IF_XAUDIO2_ENABLED(code)
#endif

#if PRISMA_ENABLE_AUDIO_OPENAL
    #define PRISMA_IF_OPENAL_ENABLED(code) code
#else
    #define PRISMA_IF_OPENAL_ENABLED(code)
#endif

#if PRISMA_ENABLE_AUDIO_SDL3
    #define PRISMA_IF_SDL3_ENABLED(code) code
#else
    #define PRISMA_IF_SDL3_ENABLED(code)
#endif

// 自动选择最佳后端
#ifdef _WIN32
    #if PRISMA_ENABLE_AUDIO_XAUDIO2
        #define PRISMA_DEFAULT_AUDIO_BACKEND XAudio2
    #elif PRISMA_ENABLE_AUDIO_OPENAL
        #define PRISMA_DEFAULT_AUDIO_BACKEND OpenAL
    #elif PRISMA_ENABLE_AUDIO_SDL3
        #define PRISMA_DEFAULT_AUDIO_BACKEND SDL3
    #else
        #define PRISMA_DEFAULT_AUDIO_BACKEND Null
    #endif
#elif defined(__APPLE__)
    #if PRISMA_ENABLE_AUDIO_OPENAL
        #define PRISMA_DEFAULT_AUDIO_BACKEND OpenAL
    #elif PRISMA_ENABLE_AUDIO_SDL3
        #define PRISMA_DEFAULT_AUDIO_BACKEND SDL3
    #else
        #define PRISMA_DEFAULT_AUDIO_BACKEND Null
    #endif
#elif defined(__ANDROID__)
    #if PRISMA_ENABLE_AUDIO_OPENAL
        #define PRISMA_DEFAULT_AUDIO_BACKEND OpenAL
    #elif PRISMA_ENABLE_AUDIO_SDL3
        #define PRISMA_DEFAULT_AUDIO_BACKEND SDL3
    #else
        #define PRISMA_DEFAULT_AUDIO_BACKEND Null
    #endif
#else
    #if PRISMA_ENABLE_AUDIO_OPENAL
        #define PRISMA_DEFAULT_AUDIO_BACKEND OpenAL
    #elif PRISMA_ENABLE_AUDIO_SDL3
        #define PRISMA_DEFAULT_AUDIO_BACKEND SDL3
    #elif PRISMA_ENABLE_AUDIO_XAUDIO2
        #define PRISMA_DEFAULT_AUDIO_BACKEND XAudio2
    #else
        #define PRISMA_DEFAULT_AUDIO_BACKEND Null
    #endif
#endif