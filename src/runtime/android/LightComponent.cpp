#include "LightComponent.h"
#include "GameObject.h"
#include "Transform.h"

namespace PrismaEngine {

Vector3 DirectionalLight::GetDirection() const {
    if (auto owner = GetOwner()) {
        if (auto transform = owner->GetTransform()) {
            // 默认方向是向前 (0, 0, 1)，应用旋转后的方向
            Vector3 forward = transform->GetForward();
            return glm::normalize(forward);
        }
    }
    // 默认向下照射
    return Vector3(0.0f, -1.0f, 0.0f);
}

} // namespace PrismaEngine
