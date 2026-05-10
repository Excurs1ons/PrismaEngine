#include "ECSApplication.h"

namespace Prisma {

ECSApplication::ECSApplication(const ApplicationSpecification& spec)
    : Application(spec)
{
}

void ECSApplication::OnUpdate(Timestep ts)
{
    // 首先调用基类更新（处理 LayerStack 等）
    Application::OnUpdate(ts);

    // 驱动 ECS 世界更新
    m_World.Update(ts);
}

} // namespace Prisma
