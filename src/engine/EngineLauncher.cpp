#include "EngineLauncher.h"
#include "Application.h"

namespace Prisma {

int RunApplication(std::unique_ptr<Application> app, const EngineSpecification& spec) {
    if (!app) {
        return -1;
    }

    Engine engine(spec);
    if (engine.Initialize() != 0) {
        return -1;
    }

    int result = engine.Run(std::move(app));
    engine.Shutdown();
    return result;
}

int RunApplication(Application* app, const EngineSpecification& spec) {
    return RunApplication(std::unique_ptr<Application>(app), spec);
}

} // namespace Prisma
