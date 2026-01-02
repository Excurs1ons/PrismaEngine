#pragma once

#include "Component.h"
#include "GameObject.h"
#include "math/MathTypes.h"

namespace PrismaEngine {

/**
 * 自动旋转组件
 * 使游戏对象每帧自动旋转
 */
class RotationComponent : public Component {
public:
    RotationComponent() = default;

    /**
     * 设置旋转速度（度/秒）
     * @param x X轴旋转速度
     * @param y Y轴旋转速度
     * @param z Z轴旋转速度
     */
    void setRotationSpeed(float x, float y, float z) {
        rotationSpeed_ = Vector3(x, y, z);
    }

    void Initialize() override {
        // 初始化时获取当前 transform
    }

    void Update(float deltaTime) override {
        if (auto* go = GetOwner()) {
            auto trans = go->GetTransform();
            // 累加旋转角度
            trans->eulerAngles += rotationSpeed_ * deltaTime;
            // 将欧拉角转换为四元数
            trans->rotation = Math::FromEulerAngles(glm::vec3(
                glm::radians(trans->eulerAngles.x),
                glm::radians(trans->eulerAngles.y),
                glm::radians(trans->eulerAngles.z)
            ));
        }
    }

private:
    Vector3 rotationSpeed_{0.0f, 0.0f, 0.0f};  // 旋转速度（度/秒）
};

} // namespace PrismaEngine
