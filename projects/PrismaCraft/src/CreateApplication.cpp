#include "PrismaCraftApp.h"

extern "C" {
GAME_API Prisma::Application* CreateApplication() {
    return new Prisma::PrismaCraftApp();
}
}
