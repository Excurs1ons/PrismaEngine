#include "Environment.h"
#include "../engine/Platform.h"
#include "../engine/Logger.h"

namespace Prisma {

EnvironmentType Environment::DetectEnvironment() {
    if (Platform::HasDisplaySupport()) {
        return EnvironmentType::Desktop;
    }
    return EnvironmentType::Headless;
}

bool Environment::HasDisplaySupport() {
    return Platform::HasDisplaySupport();
}

bool Environment::IsRunningInTerminal() {
    return Platform::IsRunningInTerminal();
}

} // namespace Prisma
