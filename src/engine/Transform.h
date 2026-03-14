#pragma once
#include "Component.h"
#include "Logger.h"
#include "math/MathTypes.h"
namespace Prisma {
class GameObject;  // 前向声明以避免循环依赖

class Transform : public Component
{
public:
    Transform() : m_Position(0.0f), m_Rotation(1.0f, 0.0f, 0.0f, 0.0f), m_Scale(1.0f), m_Matrix(1.0f), m_Dirty(true) {}

    // Setters that trigger dirty flag
    void SetPosition(const Vector3& pos) { m_Position = pos; m_Dirty = true; }
    void SetRotation(const Quaternion& rot) { m_Rotation = rot; m_Dirty = true; }
    void SetRotation(const Vector3& euler) { m_Rotation = Quaternion(glm::radians(euler)); m_Dirty = true; }
    void SetScale(const Vector3& s) { m_Scale = s; m_Dirty = true; }

    [[nodiscard]] const Vector3& GetPosition() const { return m_Position; }
    [[nodiscard]] const Quaternion& GetRotation() const { return m_Rotation; }
    [[nodiscard]] const Vector3& GetScale() const { return m_Scale; }

    // 按需计算矩阵，缓存结果
    [[nodiscard]] const Matrix4x4& GetMatrix() {
        if (m_Dirty) {
            UpdateMatrix();
        }
        return m_Matrix;
    }

    Vector3 GetForward() const {
        return m_Rotation * Vector3(0.0f, 0.0f, 1.0f);
    }

private:
    void UpdateMatrix() {
        Matrix4x4 translationMatrix = glm::translate(Matrix4x4(1.0f), m_Position);
        Matrix4x4 rotationMatrix = glm::mat4_cast(m_Rotation);
        Matrix4x4 scaleMatrix = glm::scale(Matrix4x4(1.0f), m_Scale);

        m_Matrix = translationMatrix * rotationMatrix * scaleMatrix;
        m_Dirty = false;
    }

    Vector3 m_Position;
    Quaternion m_Rotation;
    Vector3 m_Scale;
    
    Matrix4x4 m_Matrix;
    bool m_Dirty;
};
}