#include "Transform2D.h"

Transform2D::Transform2D()
    : m_position(XMVectorZero())
    , m_rotation(0.0f)
    , m_scale(XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f))
    , m_matrix(XMMatrixIdentity())
    , m_isDirty(true)
{
}

Transform2D::~Transform2D()
{
}

XMVECTOR Transform2D::GetPosition() const
{
    return m_position;
}

void Transform2D::SetPosition(FXMVECTOR position)
{
    m_position = position;
    m_isDirty = true;
}

void Transform2D::SetPosition(float x, float y, float z)
{
    m_position = XMVectorSet(x, y, z, 0.0f);
    m_isDirty = true;
}

float Transform2D::GetRotation() const
{
    return m_rotation;
}

void Transform2D::SetRotation(float rotation)
{
    m_rotation = rotation;
    m_isDirty = true;
}

XMVECTOR Transform2D::GetScale() const
{
    return m_scale;
}

void Transform2D::SetScale(FXMVECTOR scale)
{
    m_scale = scale;
    m_isDirty = true;
}

void Transform2D::SetScale(float xy)
{
    m_scale = XMVectorSet(xy, xy, 1.0f, 1.0f);
    m_isDirty = true;
}

void Transform2D::SetScale(float x, float y, float z)
{
    m_scale = XMVectorSet(x, y, z, 1.0f);
    m_isDirty = true;
}

XMMATRIX Transform2D::GetMatrix() const
{
    if (m_isDirty)
    {
        // 计算变换矩阵: Scale * Rotation * Translation
        XMMATRIX scaleMatrix = XMMatrixScalingFromVector(m_scale);
        XMMATRIX rotationMatrix = XMMatrixRotationZ(m_rotation);
        XMMATRIX translationMatrix = XMMatrixTranslationFromVector(m_position);

        m_matrix = XMMatrixMultiply(scaleMatrix, rotationMatrix);
        m_matrix = XMMatrixMultiply(m_matrix, translationMatrix);
        
        m_isDirty = false;
    }
    
    return m_matrix;
}

void Transform2D::Update(float deltaTime)
{
    // 基础变换不需要每帧更新
    // 但如果需要实现插值或其他动态效果，可以在这里处理
}