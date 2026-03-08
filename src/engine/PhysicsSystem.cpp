#include "PhysicsSystem.h"
#include "Logger.h"

namespace PrismaEngine {

std::shared_ptr<PhysicsSystem> PhysicsSystem::GetInstance() {
    static std::shared_ptr<PhysicsSystem> instance = std::make_shared<PhysicsSystem>();
    return instance;
}

int PhysicsSystem::Initialize() {
    LOG_INFO("Physics", "物理系统初始化开始");
    LOG_INFO("Physics", "物理系统初始化完成");
    return 0;
}

void PhysicsSystem::Shutdown() {
    LOG_INFO("Physics", "物理系统开始关闭");
}

void PhysicsSystem::Update(float deltaTime) {
    if (deltaTime <= 0.0f) return;
}

}  // namespace PrismaEngine
