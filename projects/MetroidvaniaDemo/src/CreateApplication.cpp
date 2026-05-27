#include "MetroidvaniaApp.h"

extern "C" {

GAME_API Prisma::Application* CreateApplication() {
    return new Prisma::MetroidvaniaApp();
}

} // extern "C"
