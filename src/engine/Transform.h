#pragma once
#include "Component.h"
#include "Logger.h"
#include <DirectXMath.h>
#include <DirectXCollision.h>
// 引入四元数
#include "Quaternion.h"
#include <cmath>

class GameObject;  // 前向声明以避免循环依赖

using namespace DirectX;

class Transform : public Component
{
public:
    // 添加简单的变换属性
    XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
    XMFLOAT3 eulerAngles = { 0.0f, 0.0f, 0.0f };
    Quaternion rotation = Quaternion::Identity;
    XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

    XMFLOAT3 GetPosition() const;
    // 获取变换矩阵
    float* GetMatrix() {
        // 创建旋转矩阵（欧拉角转换为四元数，然后转换为矩阵）
        // 注意：DirectX使用弧度制，但我们的rotation是度数
        XMVECTOR rotationQuaternion = XMQuaternionRotationRollPitchYaw(
            XMConvertToRadians(rotation.x),
            XMConvertToRadians(rotation.y),
            XMConvertToRadians(rotation.z)
        );
        XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotationQuaternion);

        // 创建平移矩阵
        XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);

        // 创建缩放矩阵
        XMMATRIX scaleMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);

        // 组合变换：先缩放，再旋转，最后平移 (S * R * T)
        XMMATRIX worldMatrix = XMMatrixMultiply(scaleMatrix, rotationMatrix);
        worldMatrix = XMMatrixMultiply(worldMatrix, translationMatrix);

        // 不在这里转置矩阵，让渲染管线统一处理
        XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(m_matrix), worldMatrix);

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