#pragma once

#if defined(_WIN32) || defined(_MSC_VER)
    #if defined(TEMPLATE2D_EXPORTS)
        #define TEMPLATE2D_API __declspec(dllexport)
    #else
        #define TEMPLATE2D_API __declspec(dllimport)
    #endif
#else
    #define TEMPLATE2D_API __attribute__((visibility("default")))
#endif
