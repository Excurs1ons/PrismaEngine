#pragma once
#include "Camera.h"
#include "Component.h"

using namespace DirectX;

class Camera3D :
    public Camera
{
public:
    Camera3D();
    ~Camera3D();

    // Component接口实现
    void Owner(GameObject* owner) override { m_owner = owner; }
    void Initialize() override;
    void Update(float deltaTime) override;

    // 设置摄像机位置
    void SetPosition(float x, float y, float z);
    void SetPosition(FXMVECTOR position);
    XMVECTOR GetPosition() const;

    // 设置摄像机旋转（欧拉角，度数）
    void SetRotation(float pitch, float yaw, float roll);
    void SetPitch(float pitch);
    void SetYaw(float yaw);
    void SetRoll(float roll);
    float GetPitch() const;
    float GetYaw() const;
    float GetRoll() const;

    // 设置透视投影参数
    void SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane = 0.1f, float farPlane = 1000.0f);

    // 获取视图矩阵
    XMMATRIX GetViewMatrix() const;

    // 获取投影矩阵
    XMMATRIX GetProjectionMatrix() const;

    // 获取视图-投影矩阵
    XMMATRIX GetViewProjectionMatrix() const;

    // 更新投影矩阵以适应窗口大小
    void UpdateProjectionMatrix(float windowWidth, float windowHeight);

    // ICamera接口实现
    XMVECTOR GetForward() const override;
    XMVECTOR GetUp() const override;
    XMVECTOR GetRight() const override;

    float GetFOV() const override { return m_fov; }
    void SetFOV(float fov) override;

    float GetNearPlane() const override { return m_nearPlane; }
    float GetFarPlane() const override { return m_farPlane; }
    float GetAspectRatio() const override { return m_aspectRatio; }

    void SetNearFarPlanes(float nearPlane, float farPlane) override;
    void SetAspectRatio(float aspectRatio) override;

    bool IsActive() const override { return m_isActive; }
    void SetActive(bool active) override { m_isActive = active; }

    // 移动相机（相对于世界坐标系）
    void MoveWorld(float x, float y, float z);
    void MoveWorld(FXMVECTOR direction);

    // 移动相机（相对于相机坐标系）
    void MoveLocal(float forward, float right, float up);

    // 旋转相机
    void Rotate(float pitch, float yaw, float roll);

    // 看向目标点
    void LookAt(FXMVECTOR target);
    void LookAt(float x, float y, float z);

private:
    void UpdateViewMatrix() const;
    void UpdateVectors() const;

    XMVECTOR m_position;
    float m_pitch;  // X轴旋转（俯仰）
    float m_yaw;    // Y轴旋转（偏航）
    float m_roll;   // Z轴旋转（翻滚）

    mutable XMMATRIX m_viewMatrix;
    mutable XMMATRIX m_projectionMatrix;

    float m_fov;
    float m_aspectRatio;
    float m_nearPlane;
    float m_farPlane;

    mutable XMVECTOR m_forward;
    mutable XMVECTOR m_up;
    mutable XMVECTOR m_right;

    mutable bool m_isViewDirty;
    mutable bool m_isProjectionDirty;
    bool m_isActive = true;
};