#pragma once
#include "Component.h"
#include "Logger.h"
#include "math/MathTypes.h"
#include "math/Math.h"
#include "Quaternion.h"
#include <cmath>

class GameObject;  // 前向声明以避免循环依赖

class Transform : public Component
{
public:
    // 添加简单的变换属性
    Prisma::Vector3 position = {0.0f, 0.0f, 0.0f};
    Prisma::Vector3 eulerAngles = { 0.0f, 0.0f, 0.0f };
    Quaternion rotation = Prisma::Math::QuaternionIdentity();
    Prisma::Vector3 scale = { 1.0f, 1.0f, 1.0f };

    [[nodiscard]] Prisma::Vector3 GetPosition() const;
    // 获取变换矩阵
    [[nodiscard]] Prisma::Matrix4x4 GetMatrix() const {
        // 创建旋转矩阵（从四元数转换为矩阵）
        glm::quat quat(rotation.w, rotation.x, rotation.y, rotation.z);
        Prisma::Matrix4x4 rotationMatrix = Prisma::Math::QuaternionToMatrix(quat);

        // 创建平移矩阵
        Prisma::Matrix4x4 translationMatrix = Prisma::Math::Translation(position);

        // 创建缩放矩阵
        Prisma::Matrix4x4 scaleMatrix = Prisma::Math::Scale(scale);

        // 组合变换：先缩放，再旋转，最后平移 (S * R * T)
        const Prisma::Matrix4x4& worldMatrix =
            Prisma::Math::Multiply(Prisma::Math::Multiply(scaleMatrix, rotationMatrix), translationMatrix);

        return worldMatrix;
    }

private:
    Prisma::Matrix4x4 matrix = {};
    void UpdateMatrix();
};