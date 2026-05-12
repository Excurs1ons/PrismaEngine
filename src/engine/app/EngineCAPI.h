#pragma once

/**
 * @brief Prisma Engine C API
 *
 * 整个 Engine DLL 只导出一个入口函数：CreateInterface。
 * 运行时加载的项目通过 LoadLibrary/dlsym 获取此函数，
 * 然后传入接口名称（如 "IEngine"）获取对应的纯虚接口指针。
 *
 * 本文件始终 __declspec(dllexport)，不受 ENGINE_EXPORTS 影响。
 */

#include "interfaces/IEngine.h"

#if defined(_WIN32) || defined(_MSC_VER)
    #define PRISMA_ENGINE_C_API __declspec(dllexport)
#else
    #define PRISMA_ENGINE_C_API __attribute__((visibility("default")))
#endif

extern "C" {

/**
 * @brief 从 Engine DLL 获取指定接口
 * @param name  接口名称（如 "IEngine"）
 * @param param 接口相关上下文参数
 *              - "IEngine": EngineCreateInfo*
 * @return 接口指针，失败返回 nullptr
 */
PRISMA_ENGINE_C_API void* CreateInterface(
    const char* name,
    void*       param);

} // extern "C"
