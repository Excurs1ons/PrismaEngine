/**
 * @brief Template3D Plugin Entry — 供 PrismaLauncher 运行时加载
 */

#include "Template3DApp.h"

extern "C" {

GAME_API Prisma::Application* CreateApplication() {
    return new Prisma::Template3DApp();
}

} // extern "C"
