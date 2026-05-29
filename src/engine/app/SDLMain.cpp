#include "Engine.h"
#include "Application.h"
#include "Logger.h"

#ifdef __ANDROID__

extern Prisma::Application* Prisma::CreateApplication();

extern "C" int SDL_main(int argc, char* argv[]) {
    Prisma::EngineSpecification spec;
    spec.Name = "PrismaEngine Android";

    Prisma::Engine engine(spec);
    if (engine.Initialize() != 0) {
        return -1;
    }

    auto* app = Prisma::CreateApplication();
    int result = engine.Run(std::unique_ptr<Prisma::Application>(app));
    engine.Shutdown();

    return result;
}

#endif
