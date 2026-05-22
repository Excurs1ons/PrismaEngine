#pragma once

#include "Application.h"
#include "core/ECS.h"

namespace Prisma {

/**
 * @brief 基于 ECS 架构的应用程序基类
 */
class ENGINE_API ECSApplication : public Application {
public:
    ECSApplication(const ApplicationSpecification& spec = ApplicationSpecification());
    virtual ~ECSApplication() override = default;

    // 获取 ECS 世界
    Core::ECS::World& GetWorld() { return m_World; }

    // 重写更新逻辑以驱动 ECS 系统
    void OnUpdate(Timestep ts) override;

protected:
    Core::ECS::World m_World;
};

} // namespace Prisma
