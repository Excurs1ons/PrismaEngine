#pragma once

// Editor API 导出/导入宏
#if defined(_WIN32) || defined(_MSC_VER)
    #ifdef EDITOR_EXPORTS
        #define EDITOR_API __declspec(dllexport)
    #else
        #define EDITOR_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #ifdef EDITOR_EXPORTS
        #define EDITOR_API __attribute__((visibility("default")))
    #else
        #define EDITOR_API
    #endif
#else
    #define EDITOR_API
#endif
