#pragma once
#include "ISubSystem.h"
#include "ManagerBase.h"
#include "WorkerThread.h"
#include <memory>
#include <string>

namespace PrismaEngine {

class ENGINE_API PhysicsSystem : public ManagerBase<PhysicsSystem> {
public:
    static std::shared_ptr<PhysicsSystem> GetInstance();

    static constexpr const char* GetStaticName() { return "PhysicsSystem"; }
    int Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    PhysicsSystem() = default;
    ~PhysicsSystem() override = default;

private:
    WorkerThread m_workerThread;
};
}  // namespace PrismaEngine
