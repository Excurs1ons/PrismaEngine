#pragma once

// 强制检查导出定义
#if defined(_WIN32) || defined(_MSC_VER)
    // 检查是否正在构建 Engine 项目自身
    #if defined(ENGINE_EXPORTS) || defined(PRISMA_ENGINE_EXPORTS) || defined(PrismaEngine_EXPORTS)
        #define ENGINE_API __declspec(dllexport)
    #else
        #define ENGINE_API __declspec(dllimport)
    #endif

    // 检查是否正在构建 Editor 项目自身
    #if defined(EDITOR_EXPORTS) || defined(PRISMA_EDITOR_EXPORTS) || defined(PrismaEditor_EXPORTS)
        #define EDITOR_API __declspec(dllexport)
    #else
        #define EDITOR_API __declspec(dllimport)
    #endif
#else
    #define ENGINE_API __attribute__((visibility("default")))
    #define EDITOR_API __attribute__((visibility("default")))
#endif
