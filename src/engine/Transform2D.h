#pragma once

#include "ITransform.h"
#include "math/MathTypes.h"
#include "math/Math.h"

class Transform2D :
    public ITransform
{
public:
    Transform2D();
    ~Transform2D();

    // 获取和设置位置
    PrismaMath::vec3 GetPosition() const;
    void SetPosition(const PrismaMath::vec3& position);
    void SetPosition(float x, float y, float z = 0.0f);

    // 获取和设置旋转（绕Z轴）
    float GetRotation() const;
    void SetRotation(float rotation);

    // 获取和设置缩放
    PrismaMath::vec3 GetScale() const;
    void SetScale(const PrismaMath::vec3& scale);
    void SetScale(float xy);
    void SetScale(float x, float y, float z = 1.0f);

    // 获取变换矩阵
    PrismaMath::mat4 GetMatrix() const;

    // 更新函数
    void Update(float deltaTime) override;

private:
    PrismaMath::vec3 m_position;
    float m_rotation;  // 绕Z轴旋转（弧度）
    PrismaMath::vec3 m_scale;
    mutable PrismaMath::mat4 m_matrix;
    mutable bool m_isDirty;
};