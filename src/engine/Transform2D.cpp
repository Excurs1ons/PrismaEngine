#include "Transform2D.h"

Transform2D::Transform2D()
    : m_position(PrismaMath::vec3(0.0f, 0.0f, 0.0f))
    , m_rotation(0.0f)
    , m_scale(PrismaMath::vec3(1.0f, 1.0f, 1.0f))
    , m_matrix(PrismaMath::mat4(1.0f))
    , m_isDirty(true)
{
}

Transform2D::~Transform2D()
{
}

PrismaMath::vec3 Transform2D::GetPosition() const
{
    return m_position;
}

void Transform2D::SetPosition(const PrismaMath::vec3& position)
{
    m_position = position;
    m_isDirty = true;
}

void Transform2D::SetPosition(float x, float y, float z)
{
    m_position = PrismaMath::vec3(x, y, z);
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

PrismaMath::vec3 Transform2D::GetScale() const
{
    return m_scale;
}

void Transform2D::SetScale(const PrismaMath::vec3& scale)
{
    m_scale = scale;
    m_isDirty = true;
}

void Transform2D::SetScale(float xy)
{
    m_scale = PrismaMath::vec3(xy, xy, 1.0f);
    m_isDirty = true;
}

void Transform2D::SetScale(float x, float y, float z)
{
    m_scale = PrismaMath::vec3(x, y, z);
    m_isDirty = true;
}

PrismaMath::mat4 Transform2D::GetMatrix() const
{
    if (m_isDirty)
    {
        // 计算变换矩阵: Scale * Rotation * Translation
        PrismaMath::mat4 scaleMatrix = Prisma::Math::Scale(m_scale);
        PrismaMath::mat4 rotationMatrix = Prisma::Math::RotationZ(m_rotation);
        PrismaMath::mat4 translationMatrix = Prisma::Math::Translation(m_position);

        m_matrix = Prisma::Math::Multiply(scaleMatrix, rotationMatrix);
        m_matrix = Prisma::Math::Multiply(m_matrix, translationMatrix);

        m_isDirty = false;
    }

    return m_matrix;
}

void Transform2D::Update(float deltaTime)
{
    // 基础变换不需要每帧更新
    // 但如果需要实现插值或其他动态效果，可以在这里处理
}