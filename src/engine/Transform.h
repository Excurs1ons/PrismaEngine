#pragma once
#include "Component.h"
#include "Logger.h"
#include "math/MathTypes.h"
namespace PrismaEngine {
class GameObject;  // 前向声明以避免循环依赖

class Transform : public Component
{
public:
    friend class Component;
    // 添加简单的变换属性
    Vector3 position = {0.0f, 0.0f, 0.0f};
    Vector3 eulerAngles = { 0.0f, 0.0f, 0.0f };
    Quaternion rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    Vector3 scale = { 1.0f, 1.0f, 1.0f };

    [[nodiscard]] PrismaEngine::Vector3 GetPosition() const;
    // 获取变换矩阵
    [[nodiscard]] PrismaEngine::Matrix4x4 GetMatrix() const {
        // 创建旋转矩阵（从四元数转换为矩阵）
        glm::quat quat(rotation.w, rotation.x, rotation.y, rotation.z);
        PrismaEngine::Matrix4x4 rotationMatrix = glm::mat4_cast(quat);

        // 创建平移矩阵
        PrismaEngine::Matrix4x4 translationMatrix = glm::translate(glm::mat4(1.0f), position);

        // 创建缩放矩阵
        PrismaEngine::Matrix4x4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

        // 组合变换：先缩放，再旋转，最后平移 (T * R * S in column-major GLM)
        const PrismaEngine::Matrix4x4& worldMatrix = translationMatrix * rotationMatrix * scaleMatrix;

        return worldMatrix;
    }
    Vector3 GetForward() const;

private:
    PrismaEngine::Matrix4x4 matrix = {};
    void UpdateMatrix();
};
}