#pragma once
#include "ISubSystem.h"
#include "WorkerThread.h"
#include <memory>
#include <string>

namespace Prisma {

class ENGINE_API PhysicsSystem : public ISubSystem {
public:
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "PhysicsSystem"; }
    PhysicsSystem()           = default;
    ~PhysicsSystem() override = default;

private:
    WorkerThread m_workerThread;
};
}  // namespace Prisma
