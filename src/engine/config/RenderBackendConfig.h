#pragma once

/// @file RenderBackendConfig.h
/// @brief 渲染设备编译配置
/// 用于控制哪些渲染设备被编译到最终版本中

// ========== 渲染设备编译控制 ==========

// 默认只启用 DirectX12 (Windows 平台优化)
// 如需启用其他设备，请取消相应注释

// Windows 平台 (默认启用)
#ifndef PRISMA_ENABLE_RENDER_DX12
    #ifdef _WIN32
        #define PRISMA_ENABLE_RENDER_DX12 1    // DirectX 12 (Windows 10+)
    #else
        #define PRISMA_ENABLE_RENDER_DX12 0
    #endif
#endif

// 跨平台渲染后端 (默认禁用)
#ifndef PRISMA_ENABLE_RENDER_OPENGL
    #define PRISMA_ENABLE_RENDER_OPENGL 0      // OpenGL 4.6+
#endif

#ifndef PRISMA_ENABLE_RENDER_VULKAN
    #define PRISMA_ENABLE_RENDER_VULKAN 0     // Vulkan 1.3+
#endif

#ifndef PRISMA_ENABLE_RENDER_METAL
    #ifdef __APPLE__
        #define PRISMA_ENABLE_RENDER_METAL 0   // Metal (macOS/iOS)
    #else
        #define PRISMA_ENABLE_RENDER_METAL 0
    #endif
#endif

// Web 平台
#ifndef PRISMA_ENABLE_RENDER_WEBGPU
    #if defined(__EMSCRIPTEN__)
        #define PRISMA_ENABLE_RENDER_WEBGPU 1  // WebGPU
    #else
        #define PRISMA_ENABLE_RENDER_WEBGPU 0
    #endif
#endif

// 软件渲染器 (调试用)
#ifndef PRISMA_ENABLE_RENDER_SOFTWARE
    #define PRISMA_ENABLE_RENDER_SOFTWARE 0  // 软件渲染器
#endif

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

// ========== 平台特定优化 ==========

// Windows 特定优化
#ifdef _WIN32
    #ifndef PRISMA_RENDER_USE_PIX
        #define PRISMA_RENDER_USE_PIX 1              // PIX 调试工具支持
    #endif

    #ifndef PRISMA_RENDER_USE_GPU_VALIDATION
        #define PRISMA_RENDER_USE_GPU_VALIDATION 1   // GPU 验证层
    #endif
#endif

// Linux 特定优化
#ifdef __linux__
    #ifndef PRISMA_RENDER_USE_MESA_DEBUG
        #define PRISMA_RENDER_USE_MESA_DEBUG 1       // Mesa 调试
    #endif
#endif

// Android 特定优化
#ifdef __ANDROID__
    #ifndef PRISMA_RENDER_USE_VULKAN_PREFERED
        #define PRISMA_RENDER_USE_VULKAN_PREFERED 1  // 优先使用 Vulkan
    #endif
#endif

// macOS 特定优化
#ifdef __APPLE__
    #ifndef PRISMA_RENDER_USE_METAL_PREFERRED
        #define PRISMA_RENDER_USE_METAL_PREFERRED 1  // 优先使用 Metal
    #endif
#endif

// ========== 编译时检查 ==========

// 确保至少有一个后端可用
#if !PRISMA_ENABLE_RENDER_DX12 && \
    !PRISMA_ENABLE_RENDER_OPENGL && \
    !PRISMA_ENABLE_RENDER_VULKAN && \
    !PRISMA_ENABLE_RENDER_METAL && \
    !PRISMA_ENABLE_RENDER_WEBGPU && \
    !PRISMA_ENABLE_RENDER_SOFTWARE
    #error "至少需要启用一个渲染后端"
#endif

// 确保主后端可用
#if defined(_WIN32) && !PRISMA_ENABLE_RENDER_DX12 && !PRISMA_ENABLE_RENDER_VULKAN && !PRISMA_ENABLE_RENDER_OPENGL
    #warning "Windows 平台未启用主要渲染后端，建议启用 DirectX12 或 Vulkan"
#endif

#if defined(__APPLE__) && !PRISMA_ENABLE_RENDER_METAL && !PRISMA_ENABLE_RENDER_VULKAN && !PRISMA_ENABLE_RENDER_OPENGL
    #warning "macOS 平台未启用渲染后端，建议启用 Metal 或 OpenGL"
#endif

// ========== 便利宏 ==========

// 定义启用的后端列表
#if PRISMA_ENABLE_RENDER_DX12
    #define PRISMA_AVAILABLE_RENDER_DX12
#endif

#if PRISMA_ENABLE_RENDER_OPENGL
    #define PRISMA_AVAILABLE_RENDER_OPENGL
#endif

#if PRISMA_ENABLE_RENDER_VULKAN
    #define PRISMA_AVAILABLE_RENDER_VULKAN
#endif

#if PRISMA_ENABLE_RENDER_METAL
    #define PRISMA_AVAILABLE_RENDER_METAL
#endif

#if PRISMA_ENABLE_RENDER_WEBGPU
    #define PRISMA_AVAILABLE_RENDER_WEBGPU
#endif

#if PRISMA_ENABLE_RENDER_SOFTWARE
    #define PRISMA_AVAILABLE_RENDER_SOFTWARE
#endif

// 条件编译宏
#if PRISMA_ENABLE_RENDER_DX12
    #define PRISMA_IF_DX12_ENABLED(code) code
#else
    #define PRISMA_IF_DX12_ENABLED(code)
#endif

#if PRISMA_ENABLE_RENDER_OPENGL
    #define PRISMA_IF_OPENGL_ENABLED(code) code
#else
    #define PRISMA_IF_OPENGL_ENABLED(code)
#endif

#if PRISMA_ENABLE_RENDER_VULKAN
    #define PRISMA_IF_VULKAN_ENABLED(code) code
#else
    #define PRISMA_IF_VULKAN_ENABLED(code)
#endif

// 自动选择最佳后端
#ifdef _WIN32
    #if PRISMA_ENABLE_RENDER_DX12
        #define PRISMA_DEFAULT_RENDER_BACKEND DirectX12
    #elif PRISMA_ENABLE_RENDER_VULKAN
        #define PRISMA_DEFAULT_RENDER_BACKEND Vulkan
    #elif PRISMA_ENABLE_RENDER_OPENGL
        #define PRISMA_DEFAULT_RENDER_BACKEND OpenGL
    #else
        #define PRISMA_DEFAULT_RENDER_BACKEND Software
    #endif
#elif defined(__APPLE__)
    #if PRISMA_ENABLE_RENDER_METAL
        #define PRISMA_DEFAULT_RENDER_BACKEND Metal
    #elif PRISMA_ENABLE_RENDER_VULKAN
        #define PRISMA_DEFAULT_RENDER_BACKEND Vulkan
    #elif PRISMA_ENABLE_RENDER_OPENGL
        #define PRISMA_DEFAULT_RENDER_BACKEND OpenGL
    #else
        #define PRISMA_DEFAULT_RENDER_BACKEND Software
    #endif
#elif defined(__ANDROID__)
    #if PRISMA_ENABLE_RENDER_VULKAN
        #define PRISMA_DEFAULT_RENDER_BACKEND Vulkan
    #elif PRISMA_ENABLE_RENDER_OPENGL
        #define PRISMA_DEFAULT_RENDER_BACKEND OpenGL
    #else
        #define PRISMA_DEFAULT_RENDER_BACKEND Software
    #endif
#elif defined(__EMSCRIPTEN__)
    #if PRISMA_ENABLE_RENDER_WEBGPU
        #define PRISMA_DEFAULT_RENDER_BACKEND WebGPU
    #elif PRISMA_ENABLE_RENDER_OPENGL
        #define PRISMA_DEFAULT_RENDER_BACKEND OpenGL
    #else
        #define PRISMA_DEFAULT_RENDER_BACKEND Software
    #endif
#else
    #if PRISMA_ENABLE_RENDER_VULKAN
        #define PRISMA_DEFAULT_RENDER_BACKEND Vulkan
    #elif PRISMA_ENABLE_RENDER_OPENGL
        #define PRISMA_DEFAULT_RENDER_BACKEND OpenGL
    #elif PRISMA_ENABLE_RENDER_DX12
        #define PRISMA_DEFAULT_RENDER_BACKEND DirectX12
    #else
        #define PRISMA_DEFAULT_RENDER_BACKEND Software
    #endif
#endif

// ========== 版本要求宏 ==========

// OpenGL 版本要求
#if PRISMA_ENABLE_RENDER_OPENGL
    #ifndef PRISMA_OPENGL_REQUIRED_VERSION_MAJOR
        #define PRISMA_OPENGL_REQUIRED_VERSION_MAJOR 4
    #endif
    #ifndef PRISMA_OPENGL_REQUIRED_VERSION_MINOR
        #define PRISMA_OPENGL_REQUIRED_VERSION_MINOR 6
    #endif
#endif

// Vulkan 版本要求
#if PRISMA_ENABLE_RENDER_VULKAN
    #ifndef PRISMA_VULKAN_REQUIRED_VERSION_MAJOR
        #define PRISMA_VULKAN_REQUIRED_VERSION_MAJOR 1
    #endif
    #ifndef PRISMA_VULKAN_REQUIRED_VERSION_MINOR
        #define PRISMA_VULKAN_REQUIRED_VERSION_MINOR 3
    #endif
#endif

// DirectX 12 版本要求
#if PRISMA_ENABLE_RENDER_DX12
    #ifndef PRISMA_DX12_REQUIRED_FEATURE_LEVEL
        #define PRISMA_DX12_REQUIRED_FEATURE_LEVEL 0xc000  // 12.0
    #endif
#endif