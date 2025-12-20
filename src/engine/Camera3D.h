#pragma once
#include "Camera.h"
#include "Quaternion.h"
#include "math/MathTypes.h"


namespace Engine::Graphic {

class Camera3D : public Camera
{
public:
    Camera3D();
    ~Camera3D() override;

    // Component接口实现
    void Initialize() override;
    void Update(float deltaTime) override;

    // 设置透视投影参数
    void SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane = 0.1f, float farPlane = 1000.0f);

    // 移动相机（相对于世界坐标系）
    void MoveWorld(float x, float y, float z);
    void MoveWorld(const PrismaMath::vec3& direction);

    // 移动相机（相对于相机坐标系）
    void MoveLocal(float forward, float right, float up);

    // 旋转相机
    void Rotate(float pitch, float yaw, float roll);

    // 看向目标点
    void LookAt(const PrismaMath::vec3& target);
    void LookAt(float x, float y, float z);

    // ICamera接口实现
    PrismaMath::mat4 GetViewMatrix() const override;
    PrismaMath::mat4 GetProjectionMatrix() const override;
    PrismaMath::mat4 GetViewProjectionMatrix() const override;

    PrismaMath::vec3 GetPosition() const override;
    PrismaMath::vec3 GetForward() const override;
    PrismaMath::vec3 GetUp() const override;
    PrismaMath::vec3 GetRight() const override;

    float GetFOV() const override { return m_fov; }
    void SetFOV(float fov) override;

    float GetNearPlane() const override { return m_nearPlane; }
    float GetFarPlane() const override { return m_farPlane; }
    float GetAspectRatio() const override { return m_aspectRatio; }

    void SetNearFarPlanes(float nearPlane, float farPlane) override;
    void SetAspectRatio(float aspectRatio) override;

    bool IsActive() const override { return m_isActive; }
    void SetActive(bool active) override { m_isActive = active; }

    PrismaMath::vec4 GetClearColor() const override;
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
    PrismaMath::vec4 m_clearColor;

    // 缓存的矩阵
    mutable PrismaMath::mat4 m_viewMatrix;
    mutable PrismaMath::mat4 m_projectionMatrix;

    // 缓存的向量（从Transform计算得出）
    mutable PrismaMath::vec3 m_forward;
    mutable PrismaMath::vec3 m_up;
    mutable PrismaMath::vec3 m_right;

    // 脏标记
    mutable bool m_isViewDirty;
    mutable bool m_isProjectionDirty;
    bool m_isActive = true;
};

} // namespace Engine::Graphic
