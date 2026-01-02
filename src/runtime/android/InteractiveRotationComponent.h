#pragma once

#include "Component.h"
#include "GameObject.h"
#include "AndroidInputBackend.h"
#include "AndroidOut.h"
#include "math/MathTypes.h"
#include <chrono>

namespace PrismaEngine {

/**
 * 交互式旋转组件
 * 支持自动旋转和触控拖拽旋转
 *
 * 使用方式：
 * 1. 自动旋转：设置 rotationSpeed
 * 2. 触控旋转：用户拖拽时自动旋转物体
 */
class InteractiveRotationComponent : public Component {
public:
    // 交互模式
    enum class InteractionMode {
        AutoRotate,       // 仅自动旋转
        TouchRotate,      // 仅触控旋转
        AutoAndTouch      // 自动旋转 + 触控交互
    };

    // 旋转轴模式
    enum class AxisMode {
        Free,             // 自由旋转（X 和 Y）
        XOnly,            // 仅 X 轴
        YOnly,            // 仅 Y 轴
        Both              // X 和 Y 轴（默认）
    };

    InteractiveRotationComponent() = default;

    /**
     * 设置交互模式
     */
    void SetInteractionMode(InteractionMode mode) {
        interactionMode_ = mode;
    }

    /**
     * 设置旋转轴模式
     */
    void SetAxisMode(AxisMode mode) {
        axisMode_ = mode;
    }

    /**
     * 设置自动旋转速度（度/秒）
     */
    void SetRotationSpeed(float x, float y, float z) {
        autoRotationSpeed_ = Vector3(x, y, z);
    }

    /**
     * 设置触控旋转灵敏度
     * 值越大，旋转越快
     */
    void SetTouchSensitivity(float sensitivity) {
        touchSensitivity_ = sensitivity;
    }

    /**
     * 设置是否需要点击物体才能旋转
     * 如果为 false，任何位置的拖拽都会旋转物体
     */
    void SetRequireTouchOnObject(bool require) {
        requireTouchOnObject_ = require;
    }

    /**
     * 设置平滑阻尼（0-1）
     * 0 = 无阻尼（立即停止）
     * 1 = 最大阻尼（继续滑动）
     */
    void SetDamping(float damping) {
        damping_ = glm::clamp(damping, 0.0f, 0.95f);
    }

    void Initialize() override {
        // 记录初始角度
        if (auto* go = GetOwner()) {
            auto trans = go->GetTransform();
            initialRotation_ = trans->eulerAngles;
        }
    }

    void Update(float deltaTime) override {
        if (auto* go = GetOwner()) {
            auto trans = go->GetTransform();

            // 处理触控输入
            if (interactionMode_ != InteractionMode::AutoRotate) {
                HandleTouchInput();
            }

            // 应用旋转速度（无论是否拖拽）
            if (glm::length(velocity_) > 0.01f) {
                trans->eulerAngles += velocity_ * deltaTime;

                // 应用阻尼（只有不拖拽时才衰减）
                if (!isDragging_) {
                    velocity_ *= (1.0f - damping_);
                }

                // 调试：输出速度和阻尼效果
                static int logCounter = 0;
                if (++logCounter % 60 == 0) {  // 每秒输出一次
                    aout << "Velocity: (" << velocity_.x << ", " << velocity_.y << "), damping=" << damping_ << std::endl;
                }

                // 停止条件
                if (glm::length(velocity_) < 0.01f) {
                    velocity_ = Vector3(0, 0, 0);
                    aout << "Velocity stopped!" << std::endl;
                }
            }

            // 转换为四元数
            trans->rotation = Math::FromEulerAngles(glm::vec3(
                glm::radians(trans->eulerAngles.x),
                glm::radians(trans->eulerAngles.y),
                glm::radians(trans->eulerAngles.z)
            ));
        }
    }

private:
    /**
     * 处理触控输入
     */
    void HandleTouchInput() {
        auto& input = Input::AndroidInputBackend::GetInstance();
        int touchCount = input.GetTouchCount();

        if (touchCount > 0) {
            const Input::Touch* touch = input.GetTouch(0);
            if (touch) {
                // 检查是否是新触摸
                if (touch->phase == Input::TouchPhase::Began && !isDragging_) {
                    aout << "Touch Began! Starting drag" << std::endl;
                    isDragging_ = true;
                    // 不要清零速度，保持之前可能的惯性
                    lastTouchPosition_ = Vector2(touch->positionX, touch->positionY);
                }
                // 检查是否正在拖拽
                else if (isDragging_ && (touch->phase == Input::TouchPhase::Moved ||
                                        touch->phase == Input::TouchPhase::Stationary)) {
                    Vector2 currentPosition(touch->positionX, touch->positionY);
                    Vector2 delta = currentPosition - lastTouchPosition_;

                    // 只有真正移动时才更新速度（滑动作为加速度叠加）
                    if (glm::length(delta) > 0.001f) {
                        aout << "Moving! delta=(" << delta.x << ", " << delta.y << ")" << std::endl;

                        // 屏幕空间滑动映射到物体旋转（基于摄像机视角）
                        // 屏幕X轴左右滑动 → 绕世界Y轴旋转（水平自转）
                        // 屏幕Y轴上下滑动 → 绕世界X轴旋转（上下翻转）
                        float accelX = delta.y * touchSensitivity_;  // 屏幕Y → 绕X轴
                        float accelY = delta.x * touchSensitivity_;  // 屏幕X → 绕Y轴

                        // 加速度叠加到当前速度
                        velocity_.x += accelX;
                        velocity_.y += accelY;

                        lastTouchPosition_ = currentPosition;
                    }
                }
                // 触摸结束
                else if (touch->phase == Input::TouchPhase::Ended ||
                         touch->phase == Input::TouchPhase::Cancelled) {
                    aout << "Touch Ended/Cancelled! Keeping velocity=(" << velocity_.x << ", " << velocity_.y << ")" << std::endl;
                    isDragging_ = false;
                    // 不清零速度，让它继续惯性旋转
                }
            }
        } else {
            // 没有触摸时，速度会自然衰减
            isDragging_ = false;
        }
    }

    // 配置
    InteractionMode interactionMode_ = InteractionMode::AutoAndTouch;
    AxisMode axisMode_ = AxisMode::Both;
    Vector3 autoRotationSpeed_{30.0f, 30.0f, 0.0f};
    float touchSensitivity_ = 0.5f;
    bool requireTouchOnObject_ = false;
    float damping_ = 0.9f;

    // 运行时状态
    bool isDragging_ = false;
    Vector2 lastTouchPosition_{0, 0};
    Vector3 velocity_{0, 0, 0};  // 当前旋转速度（用于惯性）
    Vector3 initialRotation_{0, 0, 0};
};

} // namespace PrismaEngine
