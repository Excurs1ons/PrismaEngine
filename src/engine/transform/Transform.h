#pragma once
#include "Component.h"
#include "Logger.h"
#include "math/MathTypes.h"
#include <array>
namespace Prisma {

class ENGINE_API Transform : public Component
{
public:
    struct Data {
        std::array<float, 3> position = {0,0,0};
        std::array<float, 4> rotation = {0,0,0,1};  // quat x,y,z,w
        std::array<float, 3> scale = {1,1,1};
    };

    Transform() : m_Position(0.0f), m_Rotation(1.0f, 0.0f, 0.0f, 0.0f), m_Scale(1.0f), m_Matrix(1.0f), m_Dirty(true) {}

    // Setters that trigger dirty flag
    void SetPosition(const Vector3& pos) { m_Position = pos; m_Dirty = true; }
    void SetRotation(const Quaternion& rot) { m_Rotation = rot; m_Dirty = true; }
    void SetRotation(const Vector3& euler) { m_Rotation = Quaternion(glm::radians(euler)); m_Dirty = true; }
    void SetScale(const Vector3& s) { m_Scale = s; m_Dirty = true; }

    [[nodiscard]] const Vector3& GetPosition() const { return m_Position; }
    [[nodiscard]] const Quaternion& GetRotation() const { return m_Rotation; }
    [[nodiscard]] const Vector3& GetScale() const { return m_Scale; }

    // 序列化
    const char* GetComponentTypeName() const override { return "Transform"; }
    Data GetData() const;
    void SetData(const Data& d);

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

    /// 提取 Y 轴旋转角（弧度），等同于 glm::yaw(quat)
    float GetYaw() const {
        // 来自 glm::yaw(quat): atan2(2.0f * (w*y + x*z), 1.0f - 2.0f * (x*x + y*y))
        return std::atan2(2.0f * (m_Rotation.w * m_Rotation.y + m_Rotation.x * m_Rotation.z),
                          1.0f - 2.0f * (m_Rotation.x * m_Rotation.x + m_Rotation.y * m_Rotation.y));
    }

    /// 以欧拉角（弧度）返回旋转: pitch, yaw, roll
    Vector3 GetEulerAngles() const {
        // 来自 glm::eulerAngles(quat)
        float sinPitch = -2.0f * (m_Rotation.y * m_Rotation.z - m_Rotation.w * m_Rotation.x);
        float pitch = std::asin(std::clamp(sinPitch, -1.0f, 1.0f));
        float yaw   = std::atan2(2.0f * (m_Rotation.w * m_Rotation.z + m_Rotation.x * m_Rotation.y),
                                 1.0f - 2.0f * (m_Rotation.y * m_Rotation.y + m_Rotation.z * m_Rotation.z));
        float roll  = std::atan2(2.0f * (m_Rotation.w * m_Rotation.y + m_Rotation.z * m_Rotation.x),
                                 1.0f - 2.0f * (m_Rotation.x * m_Rotation.x + m_Rotation.y * m_Rotation.y));
        return Vector3(pitch, yaw, roll);
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