/**
 * @brief Template3D Plugin Entry — 供 PrismaLauncher 运行时加载
 */

#include "Template3DApp.h"

#if defined(_MSC_VER)
    #define TEMPLATE3D_PLUGIN_API __declspec(dllexport)
#else
    #define TEMPLATE3D_PLUGIN_API __attribute__((visibility("default")))
#endif

extern "C" {

TEMPLATE3D_PLUGIN_API Prisma::Application* CreateApplication() {
    return new Prisma::Template3DApp();
}

} // extern "C"
