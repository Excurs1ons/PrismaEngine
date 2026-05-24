/**
 * @brief PathTracing3D Plugin Entry — 供 PrismaLauncher 运行时加载
 */

#include "PathTracing3DApp.h"

extern "C" {

GAME_API Prisma::Application* CreateApplication() {
    return new Prisma::PathTracing3DApp();
}

} // extern "C"
