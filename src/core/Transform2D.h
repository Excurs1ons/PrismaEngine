#pragma once

#include "ITransform.h"
#include <DirectXMath.h>

using namespace DirectX;

class Transform2D :
    public ITransform
{
public:
    Transform2D();
    ~Transform2D();

    // 获取和设置位置
    XMVECTOR GetPosition() const;
    void SetPosition(FXMVECTOR position);
    void SetPosition(float x, float y, float z = 0.0f);

    // 获取和设置旋转（绕Z轴）
    float GetRotation() const;
    void SetRotation(float rotation);

    // 获取和设置缩放
    XMVECTOR GetScale() const;
    void SetScale(FXMVECTOR scale);
    void SetScale(float xy);
    void SetScale(float x, float y, float z = 1.0f);

    // 获取变换矩阵
    XMMATRIX GetMatrix() const;

    // 更新函数
    void Update(float deltaTime) override;

private:
    XMVECTOR m_position;
    float m_rotation;  // 绕Z轴旋转（弧度）
    XMVECTOR m_scale;
    mutable XMMATRIX m_matrix;
    mutable bool m_isDirty;
};