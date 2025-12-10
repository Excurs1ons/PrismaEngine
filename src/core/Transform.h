#pragma once
#include "Component.h"
#include "Logger.h"
#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;

class Transform : public Component
{
public:
    // 添加简单的变换属性
    XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
    XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

    XMFLOAT3 GetPosition() const;
    // 获取变换矩阵
    float* GetMatrix() {
        // 对于2D变换，我们只需要简单的平移
        // 创建平移矩阵
        XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);

        // 创建缩放矩阵
        XMMATRIX scaleMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);

        // 组合变换：先缩放，再平移 (S * T)
        XMMATRIX worldMatrix = XMMatrixMultiply(scaleMatrix, translationMatrix);

        // 转置矩阵以适应HLSL的列主序要求
        XMMATRIX worldMatrixTranspose = XMMatrixTranspose(worldMatrix);

        // 存储为16个浮点数的数组
        XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(m_matrix), worldMatrixTranspose);

        // 添加调试日志（仅在位置变化时输出）
        static float lastPos[3] = {9999, 9999, 9999};
        if (std::abs(position.x - lastPos[0]) > 0.01f ||
            std::abs(position.y - lastPos[1]) > 0.01f ||
            std::abs(position.z - lastPos[2]) > 0.01f) {

            LOG_DEBUG("Transform", "GameObject '{0}' pos({1:.2f}, {2:.2f}, {3:.2f}) scale({4:.2f}, {5:.2f}, {6:.2f})",
                //m_owner ? m_owner->name : "Unknown",
                "",
                position.x, position.y, position.z,
                scale.x, scale.y, scale.z);

            lastPos[0] = position.x;
            lastPos[1] = position.y;
            lastPos[2] = position.z;
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