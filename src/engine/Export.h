#pragma once

// Windows 平台使用 DLL 导出/导入
#if defined(_WIN32) || defined(_MSC_VER)
    #ifdef ENGINE_EXPORTS
        #define ENGINE_API __declspec(dllexport)
    #else
        #define ENGINE_API __declspec(dllimport)
    #endif
// Linux/Unix 平台使用 GCC/Clang 属性
#elif defined(__GNUC__) || defined(__clang__)
    #ifdef ENGINE_EXPORTS
        #define ENGINE_API __attribute__((visibility("default")))
    #else
        #define ENGINE_API
    #endif
#else
    #define ENGINE_API
#endif