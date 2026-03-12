#include "Environment.h"
#include "../engine/Logger.h"
#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <VersionHelpers.h>
#endif

namespace PrismaEngine {

EnvironmentType Environment::DetectEnvironment() {
    if (HasDisplaySupport()) {
        return EnvironmentType::Desktop;
    }
    return EnvironmentType::Headless;
}

bool Environment::HasDisplaySupport() {
#if defined(_WIN32) || defined(_WIN64)
    return DetectDisplayWindows();
#else
    return false;
#endif
}

bool Environment::IsRunningInTerminal() {
    return !IsRedirectedOutput();
}
