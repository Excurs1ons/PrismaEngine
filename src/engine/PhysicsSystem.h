#pragma once
#include "ISubSystem.h"
#include "ManagerBase.h"
#include "WorkerThread.h"
#include <memory>
#include <string>

namespace Prisma {

class ENGINE_API PhysicsSystem : public ManagerBase<PhysicsSystem> {
public:
    static std::shared_ptr<PhysicsSystem> Get();

    static constexpr const char* GetStaticName() { return "PhysicsSystem"; }
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    PhysicsSystem()           = default;
    ~PhysicsSystem() override = default;

private:
    WorkerThread m_workerThread;
};
}  // namespace Prisma
