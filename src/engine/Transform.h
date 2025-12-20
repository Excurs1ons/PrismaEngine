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
    PrismaMath::vec3 position = {0.0f, 0.0f, 0.0f};
    PrismaMath::vec3 eulerAngles = { 0.0f, 0.0f, 0.0f };
    Quaternion rotation = Quaternion::Identity;
    PrismaMath::vec3 scale = { 1.0f, 1.0f, 1.0f };

    PrismaMath::vec3 GetPosition() const;
    // 获取变换矩阵
    float* GetMatrix() {
        // 创建旋转矩阵（从四元数转换为矩阵）
        PrismaMath::mat4 rotationMatrix = Prisma::QuaternionToMatrix(rotation.ToInternalVector());

        // 创建平移矩阵
        PrismaMath::mat4 translationMatrix = Prisma::Translation(position);

        // 创建缩放矩阵
        PrismaMath::mat4 scaleMatrix = Prisma::Scale(scale);

        // 组合变换：先缩放，再旋转，最后平移 (S * R * T)
        PrismaMath::mat4 worldMatrix = Prisma::Multiply(scaleMatrix, rotationMatrix);
        worldMatrix = Prisma::Multiply(worldMatrix, translationMatrix);

        // 将矩阵数据复制到浮点数组
        memcpy(m_matrix, &worldMatrix[0][0], sizeof(float) * 16);

        // 添加调试日志（仅在位置或旋转变化时输出）
        static float lastPos[3] = {9999, 9999, 9999};
        static float lastRot[3] = {9999, 9999, 9999};
        if (std::abs(position.x - lastPos[0]) > 0.01f ||
            std::abs(position.y - lastPos[1]) > 0.01f ||
            std::abs(position.z - lastPos[2]) > 0.01f ||
            std::abs(rotation.x - lastRot[0]) > 0.01f ||
            std::abs(rotation.y - lastRot[1]) > 0.01f ||
            std::abs(rotation.z - lastRot[2]) > 0.01f) {

            lastPos[0] = position.x;
            lastPos[1] = position.y;
            lastPos[2] = position.z;
            lastRot[0] = rotation.x;
            lastRot[1] = rotation.y;
            lastRot[2] = rotation.z;
        }

        return m_matrix;
    }

private:
    float m_matrix[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
};