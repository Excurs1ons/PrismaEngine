#include "Environment.h"
#include "logger/Logger.h"
#include "platform/Platform.h"

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

}  // namespace Prisma
