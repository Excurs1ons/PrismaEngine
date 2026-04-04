#pragma once

#include "Engine.h"
#include <memory>

namespace Prisma {

class Application;

// Helper for game/runtime executables: own engine lifecycle and run one app instance.
ENGINE_API int RunApplication(std::unique_ptr<Application> app, const EngineSpecification& spec = EngineSpecification());
ENGINE_API int RunApplication(Application* app, const EngineSpecification& spec = EngineSpecification());

} // namespace Prisma
