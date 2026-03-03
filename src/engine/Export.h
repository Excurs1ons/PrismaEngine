#pragma once

// Windows 平台使用 DLL 导出/导入
#if defined(_WIN32) || defined(_MSC_VER)
    #if defined(ENGINE_BUILD_STATIC)
        // 静态库构建 - 不需要导出/导入宏
        #define ENGINE_API
    #elif defined(ENGINE_EXPORTS)
        #define ENGINE_API __declspec(dllexport)
    #else
        #define ENGINE_API __declspec(dllimport)
    #endif
// Linux/Unix 平台使用 GCC/Clang 属性
#elif defined(__GNUC__) || defined(__clang__)
    #if defined(ENGINE_EXPORTS)
        #define ENGINE_API __attribute__((visibility("default")))
    #else
        #define ENGINE_API
    #endif
#else
    #define ENGINE_API
#endif