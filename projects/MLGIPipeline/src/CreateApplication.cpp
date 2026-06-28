/**
 MLGIPipeline Plugin Entry — 供 PrismaLauncher 运行时加载
 */

#include "MLGIPipelineApp.h"

extern "C" {

GAME_API const char* GetProjectName() {
    return "MLGIPipeline";
}

GAME_API Prisma::Application* CreateApplication() {
    return new Prisma::MLGIPipelineApp();
}

} // extern "C"
