#include "Light2D.h"
#include <algorithm>

namespace Prisma {
namespace Graphic {

Light2D::Light2D(Type type) : m_type(type) {}

void Light2D::SetSpotAngle(float angleDegrees) {
    m_spotAngle = glm::clamp(angleDegrees, 0.0f, 180.0f);
}

void Light2D::Update(Timestep /*ts*/) {
    // 未来可以添加动画、闪烁等效果
}

}  // namespace Graphic
}  // namespace Prisma