#pragma once
#include <string>
#include "Export.h"

namespace PrismaEngine {

class ENGINE_API ISubSystem {
public:
    virtual ~ISubSystem()                = default;
    virtual int Initialize()             = 0;
    virtual void Shutdown()              = 0;
    virtual void Update([[maybe_unused]] float deltaTime) {}
};

}  // namespace PrismaEngine
