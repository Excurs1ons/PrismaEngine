#include "CameraComponent.h"

namespace Prisma {

CameraComponent::CameraComponent()
{
    m_Camera = std::make_shared<Graphic::OrthographicCamera>();
}

void CameraComponent::Initialize()
{
    // 如果有 Owner，可以根据 Owner 的 Transform 同步位置（可选）
}

void CameraComponent::SetProjection(float left, float right, float bottom, float top)
{
    m_Camera->SetProjection(left, right, bottom, top);
}

void CameraComponent::SetClearColor(float r, float g, float b, float a)
{
    m_Camera->SetClearColor(r, g, b, a);
}

} // namespace Prisma
