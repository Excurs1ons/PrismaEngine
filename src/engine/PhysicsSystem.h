#pragma once
#include "ISubSystem.h"
#include "ManagerBase.h"
#include "WorkerThread.h"
#include <string>

namespace Engine {

class PhysicsSystem : public ManagerBase<PhysicsSystem> {
public:
    friend class ISubSystem;
    friend class ManagerBase<PhysicsSystem>;
    static constexpr std::string GetName() { return R"(PhysicsSystem)"; }
    bool Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;

private:
    WorkerThread workerThread;
};
}  // namespace Engine