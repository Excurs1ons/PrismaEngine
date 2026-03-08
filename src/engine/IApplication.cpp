#include "IApplication.h"

namespace PrismaEngine {

IApplicationBase::IApplicationBase() : isRunning(false), isInitialized(false) {}
IApplicationBase::~IApplicationBase() = default;

bool IApplicationBase::IsRunning() const {
    return isRunning;
}

void IApplicationBase::SetRunning(bool running) {
    isRunning = running;
}

} // namespace PrismaEngine
