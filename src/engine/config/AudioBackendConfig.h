#pragma once

/// @file AudioBackendConfig.h
/// @brief 音频设备编译配置 (Prune分支简化版)
/// Prisma Engine (prune分支) 仅支持 SDL3 音频后端

// ========== 音频设备编译控制 ==========

#define PRISMA_ENABLE_AUDIO_SDL3 1         // SDL3 Audio (默认后端)

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

// ========== 便利宏 ==========

#define PRISMA_IF_SDL3_ENABLED(code) code
#define PRISMA_DEFAULT_AUDIO_BACKEND SDL3
