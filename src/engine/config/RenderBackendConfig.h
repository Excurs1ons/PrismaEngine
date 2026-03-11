#pragma once

/// @file RenderBackendConfig.h
/// @brief 渲染设备编译配置 (Prune分支简化版)
/// Prisma Engine (prune分支) 仅支持 Vulkan 渲染后端

// ========== 渲染设备编译控制 ==========

// 剪枝分支配置：仅启用 Vulkan
#define PRISMA_ENABLE_RENDER_DX12 0        // DirectX12 已禁用
#define PRISMA_ENABLE_RENDER_VULKAN 1       // Vulkan 1.3+ (默认后端)
#define PRISMA_ENABLE_RENDER_OPENGL 0       // OpenGL 已禁用
#define PRISMA_ENABLE_RENDER_METAL 0        // Metal 已禁用
#define PRISMA_ENABLE_RENDER_WEBGPU 0       // WebGPU 已禁用
#define PRISMA_ENABLE_RENDER_SOFTWARE 0      // 软件渲染器已禁用

// ========== 渲染功能宏 ==========

#ifndef PRISMA_ENABLE_RASTERIZATION
    #define PRISMA_ENABLE_RASTERIZATION 1    // 光栅化渲染
#endif

#ifndef PRISMA_ENABLE_RAYTRACING
    #define PRISMA_ENABLE_RAYTRACING 0       // 硬件光线追踪
#endif

#ifndef PRISMA_ENABLE_MESH_SHADERS
    #define PRISMA_ENABLE_MESH_SHADERS 0      // 网格着色器
#endif

#ifndef PRISMA_ENABLE_VARIABLE_RATE_SHADING
    #define PRISMA_ENABLE_VARIABLE_RATE_SHADING 0  // 可变速率着色
#endif

#ifndef PRISMA_ENABLE_COMPUTE_SHADERS
    #define PRISMA_ENABLE_COMPUTE_SHADERS 1   // 计算着色器
#endif

#ifndef PRISMA_ENABLE_GEOMETRY_SHADERS
    #define PRISMA_ENABLE_GEOMETRY_SHADERS 1  // 几何着色器
#endif

#ifndef PRISMA_ENABLE_TESSELLATION_SHADERS
    #define PRISMA_ENABLE_TESSELLATION_SHADERS 1  // 曲面细分着色器
#endif

// ========== 渲染特性宏 ==========

#ifndef PRISMA_ENABLE_MULTITHREADED_RENDERING
    #define PRISMA_ENABLE_MULTITHREADED_RENDERING 1  // 多线程渲染
#endif

#ifndef PRISMA_ENABLE_BINDLESS_RESOURCES
    #define PRISMA_ENABLE_BINDLESS_RESOURCES 0       // 无绑定资源
#endif

#ifndef PRISMA_ENABLE_ASYNC_COMPUTE
    #define PRISMA_ENABLE_ASYNC_COMPUTE 0            // 异步计算
#endif

#ifndef PRISMA_ENABLE_GBUFFER
    #define PRISMA_ENABLE_GBUFFER 1                   // G-Buffer (延迟渲染)
#endif

#ifndef PRISMA_ENABLE_HDR
    #define PRISMA_ENABLE_HDR 1                       // HDR 渲染
#endif

#ifndef PRISMA_ENABLE_TONEMAPPING
    #define PRISMA_ENABLE_TONEMAPPING 1              // 色调映射
#endif

#ifndef PRISMA_ENABLE_BLOOM
    #define PRISMA_ENABLE_BLOOM 1                    // 泛光效果
#endif

#ifndef PRISMA_ENABLE_SSR
    #define PRISMA_ENABLE_SSR 0                      // 屏幕空间反射
#endif

#ifndef PRISMA_ENABLE_SSGI
    #define PRISMA_ENABLE_SSGI 0                     // 屏幕空间全局光照
#endif

// ========== Windows x64 特定优化 ==========

#ifdef _WIN32
    #ifndef PRISMA_RENDER_USE_PIX
        #define PRISMA_RENDER_USE_PIX 1              // PIX 调试工具支持
    #endif

    #ifndef PRISMA_RENDER_USE_GPU_VALIDATION
        #define PRISMA_RENDER_USE_GPU_VALIDATION 1   // GPU 验证层
    #endif
#endif

// ========== 便利宏 ==========

// 定义启用的后端列表
#define PRISMA_AVAILABLE_RENDER_VULKAN

// 条件编译宏
#define PRISMA_IF_VULKAN_ENABLED(code) code

// 自动选择最佳后端
#define PRISMA_DEFAULT_RENDER_BACKEND Vulkan

// ========== 版本要求宏 ==========

// Vulkan 版本要求
#define PRISMA_VULKAN_REQUIRED_VERSION_MAJOR 1
#define PRISMA_VULKAN_REQUIRED_VERSION_MINOR 3
