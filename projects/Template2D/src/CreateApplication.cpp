/**
 * @brief Template2D Plugin Entry — 供 PrismaLauncher 运行时加载
 *
 * PrismaLauncher 通过 LoadLibrary("Template2D.dll") 加载本 DLL，
 * 然后调用 CreateApplication() 获取 Application 实例。
 */

#include "Template2DApp.h"

// 内联导出宏（不依赖独立头文件）
#if defined(_MSC_VER)
    #define TEMPLATE2D_PLUGIN_API __declspec(dllexport)
#else
    #define TEMPLATE2D_PLUGIN_API __attribute__((visibility("default")))
#endif

extern "C" {

TEMPLATE2D_PLUGIN_API Prisma::Application* CreateApplication() {
    return new Prisma::Template2DApp();
}

} // extern "C"
