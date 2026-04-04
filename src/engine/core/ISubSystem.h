#pragma once
#include <string>
#include "Export.h"

#include "core/Timestep.h"

namespace Prisma {

class ENGINE_API ISubSystem {
public:
    virtual ~ISubSystem()                = default;
    virtual int Initialize()             = 0;
    virtual void Shutdown()              = 0;
    virtual void Update([[maybe_unused]] Timestep ts) {}
};

}  // namespace Prisma
