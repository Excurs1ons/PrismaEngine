#include "Template2DApp.h"

extern "C" {

GAME_API Prisma::Application* CreateApplication() {
    return new Prisma::Template2DApp();
}

} // extern "C"
