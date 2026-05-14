#include "PrismaCraftApp.h"

#if defined(_MSC_VER)
    #define PRISMACRAFT_PLUGIN_API __declspec(dllexport)
#else
    #define PRISMACRAFT_PLUGIN_API __attribute__((visibility("default")))
#endif

extern "C" {
PRISMACRAFT_PLUGIN_API Prisma::Application* CreateApplication() {
    return new Prisma::PrismaCraftApp();
}
}
