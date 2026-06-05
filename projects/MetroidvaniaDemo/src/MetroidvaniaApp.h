#pragma once

#include "app/Application.h"
#include "core/Node.h"

namespace Prisma {
namespace Physics { class RigidBody; }

class MetroidvaniaApp : public Application {
public:
    MetroidvaniaApp();
    ~MetroidvaniaApp() override = default;

    void SetAutoQuit(bool quit) { m_autoQuit = quit; }

    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    void SyncPhysicsToNodes();

    bool m_autoQuit = false;
    float m_elapsedTime = 0;
    float m_autoExitTimeout = 30.0f;

    // 物理刚体引用（用于每帧同步位置到渲染节点）
    Physics::RigidBody* m_redBody = nullptr;
    Node m_redNode;
};

} // namespace Prisma