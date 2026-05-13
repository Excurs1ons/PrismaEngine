#include "PhysicsSystem.h"
#include "Logger.h"
#include "SceneManager.h"
#include "Engine.h"
#include "GameObject.h"
#include "physics/PhysicsComponents.h"

namespace Prisma {

int PhysicsSystem::Initialize() {
    LOG_INFO("Physics", "Prisma 物理子系统正在初始化...");
    return 0;
}

void PhysicsSystem::Shutdown() {
    LOG_INFO("Physics", "物理子系统正在关闭。");
}

void PhysicsSystem::Update(Timestep ts) {
    float dt = ts.GetSeconds();
    if (dt <= 0.0f) return;

    auto scene = Engine::Get().GetSceneManager()->GetCurrentScene();
    if (!scene) return;

    const Vector3 gravity{0.0f, -9.81f, 0.0f};

    /* 暂时屏蔽，等待 Scene/Node 重构完成
    // 1. 物理积分更新
    for (auto& obj : scene->GetGameObjects()) {
        // ...
    }
    */
}

} // namespace Prisma
