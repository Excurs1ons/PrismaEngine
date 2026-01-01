#pragma once

#include "Camera.h"
#include "math/MathTypes.h"
#include "Component.h"
#include "graphic/ICamera.h"

class CameraBase : public Component, public PrismaEngine::Graphic::ICamera {
public:
    // 设置和获取清除颜色
    void SetClearColor(float r, float g, float b, float a = 1.0f) { m_clearColor = PrismaMath::vec4(r, g, b, a); }
    PrismaMath::vec4 GetClearColor() { return m_clearColor; }

protected:
    // 清除颜色，默认为青色
    PrismaMath::vec4 m_clearColor = PrismaMath::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Default to black (RGBA)
};


namespace PrismaEngine::Graphic {

class Camera : public CameraBase
{
public:
    Camera();
    ~Camera() override;

    // Component接口实现
    void Initialize() override;
    void Update(float deltaTime) override;

    // 设置透视投影参数
    void SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane = 0.1f, float farPlane = 1000.0f);

    // 移动相机（相对于世界坐标系）
    void MoveWorld(float x, float y, float z);
    void MoveWorld(const PrismaEngine::Vector3& direction);

    // 移动相机（相对于相机坐标系）
    void MoveLocal(float forward, float right, float up);

    // 旋转相机
    void Rotate(float pitch, float yaw, float roll);

    // 看向目标点
    void LookAt(const PrismaEngine::Vector3& target);
    void LookAt(float x, float y, float z);

    // ICamera接口实现
    PrismaEngine::Matrix4x4 GetViewMatrix() const override;
    PrismaEngine::Matrix4x4 GetProjectionMatrix() const override;
    PrismaEngine::Matrix4x4 GetViewProjectionMatrix() const override;

    PrismaEngine::Vector3 GetPosition() const override;
    PrismaEngine::Vector3 GetForward() const override;
    PrismaEngine::Vector3 GetUp() const override;
    PrismaEngine::Vector3 GetRight() const override;

    float GetFOV() const override { return m_fov; }
    void SetFOV(float fov) override;

    float GetNearPlane() const override { return m_nearPlane; }
    float GetFarPlane() const override { return m_farPlane; }
    float GetAspectRatio() const override { return m_aspectRatio; }

    void SetNearFarPlanes(float nearPlane, float farPlane) override;
    void SetAspectRatio(float aspectRatio) override;

    bool IsActive() const override { return m_isActive; }
    void SetActive(bool active) override { m_isActive = active; }

    PrismaEngine::Vector4 GetClearColor() const override;
    void SetClearColor(float r, float g, float b, float a = 1.0f) override;

private:
    void UpdateViewMatrix() const;
    void UpdateVectors() const;
    void MarkViewDirty() const;

    // 投影参数
    float m_fov;
    float m_aspectRatio;
    float m_nearPlane;
    float m_farPlane;

    // 清除颜色
    PrismaEngine::Vector4 m_clearColor;

    // 缓存的矩阵
    mutable PrismaEngine::Matrix4x4 m_viewMatrix;
    mutable PrismaEngine::Matrix4x4 m_projectionMatrix;

    // 缓存的向量（从Transform计算得出）
    mutable PrismaEngine::Vector3 m_forward;
    mutable PrismaEngine::Vector3 m_up;
    mutable PrismaEngine::Vector3 m_right;

    // 脏标记
    mutable bool m_isViewDirty;
    mutable bool m_isProjectionDirty;
    bool m_isActive = true;
};

} // namespace Engine::Graphic
