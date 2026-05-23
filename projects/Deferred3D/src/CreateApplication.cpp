#include "Deferred3DApp.h"

extern "C" {

GAME_API Prisma::Application* CreateApplication()
{
    return new Prisma::Deferred3DApp();
}

} // extern "C"
