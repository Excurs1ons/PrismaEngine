#pragma once
#include "Component.h"
#include "Logger.h"
#include "math/MathTypes.h"

class GameObject;  // 前向声明以避免循环依赖

class Transform : public Component
{
public:
    // 添加简单的变换属性
    PrismaEngine::Vector3 position = {0.0f, 0.0f, 0.0f};
    PrismaEngine::Vector3 eulerAngles = { 0.0f, 0.0f, 0.0f };
    Quaternion rotation = PrismaEngine::Math::QuaternionIdentity();
    PrismaEngine::Vector3 scale = { 1.0f, 1.0f, 1.0f };

    [[nodiscard]] PrismaEngine::Vector3 GetPosition() const;
    // 获取变换矩阵
    [[nodiscard]] PrismaEngine::Matrix4x4 GetMatrix() const {
        // 创建旋转矩阵（从四元数转换为矩阵）
        glm::quat quat(rotation.w, rotation.x, rotation.y, rotation.z);
        PrismaEngine::Matrix4x4 rotationMatrix = PrismaEngine::Math::QuaternionToMatrix(quat);

        // 创建平移矩阵
        PrismaEngine::Matrix4x4 translationMatrix = PrismaEngine::Math::Translation(position);

        // 创建缩放矩阵
        PrismaEngine::Matrix4x4 scaleMatrix = PrismaEngine::Math::Scale(scale);

        // 组合变换：先缩放，再旋转，最后平移 (S * R * T)
        const PrismaEngine::Matrix4x4& worldMatrix =
            PrismaEngine::Math::Multiply(PrismaEngine::Math::Multiply(scaleMatrix, rotationMatrix), translationMatrix);

        return worldMatrix;
    }

private:
    PrismaEngine::Matrix4x4 matrix = {};
    void UpdateMatrix();
};