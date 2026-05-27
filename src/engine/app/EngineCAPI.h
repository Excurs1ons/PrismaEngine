#pragma once

/* Prisma Engine C API */

#include "interfaces/IEngine.h"

#if defined(_WIN32) || defined(_MSC_VER)
    #define PRISMA_ENGINE_C_API __declspec(dllexport)
#else
    #define PRISMA_ENGINE_C_API __attribute__((visibility("default")))
#endif

extern "C" {

/* 从 Engine DLL 获取指定接口 */
PRISMA_ENGINE_C_API void* CreateInterface(
    const char* name,
    void*       param);

} // extern "C"
