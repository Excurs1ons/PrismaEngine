/**
 * @brief Template2D Plugin Entry — 供 PrismaLauncher 运行时加载
 *
 * 不再有独立的 main()。PrismaLauncher 通过 LoadLibrary("Template2D.dll")
 * 加载本 DLL，然后调用 CreateApplication() 获取 Application 实例。
 */

#include "Template2DApp.h"
#include "Template2DExport.h"

extern "C" {

TEMPLATE2D_API Prisma::Application* CreateApplication() {
    return new Prisma::Template2DApp();
}

} // extern "C"
