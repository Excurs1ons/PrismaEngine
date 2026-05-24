#include "Prisma2DApp.h"

extern "C" {

GAME_API Prisma::Application* CreateApplication() {
    return new Prisma::Prisma2DApp();
}

} // extern "C"
