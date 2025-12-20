#pragma once
#include "Camera.h"
#include "Component.h"
#include "math/MathTypes.h"

class Camera2D :
    public Camera
{
public:
    Camera2D();
    ~Camera2D() override;

    // Component接口实现
    void Owner(GameObject* owner) override { m_owner = owner; }
    void Initialize() override;
    void Update(float deltaTime) override;

    // 设置摄像机位置
    void SetPosition(float x, float y, float z = 0.0f);
    void SetPosition(const Prisma::Vector3& position);
    Prisma::Vector3 GetPosition() const override;

    // 设置摄像机旋转（绕Z轴）
    void SetRotation(float rotation);
    float GetRotation() const;

    // 设置正交投影参数
    void SetOrthographicProjection(float width, float height, float nearPlane = 0.1f, float farPlane = 1000.0f);
    void SetOrthographicProjection(float left, float right, float bottom, float top, float nearPlane = 0.1f, float farPlane = 1000.0f);


    // 获取视图矩阵
    Prisma::Matrix4x4 GetViewMatrix() const override;

    // 获取投影矩阵
    Prisma::Matrix4x4 GetProjectionMatrix() const override;

    // 获取视图-投影矩阵
    Prisma::Matrix4x4 GetViewProjectionMatrix() const override;

    // 更新投影矩阵以适应窗口大小
    void UpdateProjectionMatrix(float windowWidth, float windowHeight);

    // ICamera接口实现
    Prisma::Vector3 GetForward() const override { return Prisma::Vector3(0.0f, 0.0f, -1.0f); }
    Prisma::Vector3 GetUp() const override { return Prisma::Vector3(0.0f, 1.0f, 0.0f); }
    Prisma::Vector3 GetRight() const override { return Prisma::Vector3(1.0f, 0.0f, 0.0f); }

    float GetFOV() const override { return 0.0f; } // 正交投影没有FOV
    void SetFOV(float fov) override { (void)fov; } // 正交投影忽略FOV

    float GetNearPlane() const override { return m_nearPlane; }
    float GetFarPlane() const override { return m_farPlane; }
    float GetAspectRatio() const override { return m_width / m_height; }

    void SetNearFarPlanes(float nearPlane, float farPlane) override;
    void SetAspectRatio(float aspectRatio) override;

    bool IsActive() const override { return m_isActive; }
    void SetActive(bool active) override { m_isActive = active; }

private:
    Prisma::Vector3 m_position;
    float m_rotation;

    mutable Prisma::Matrix4x4 m_viewMatrix;
    mutable Prisma::Matrix4x4 m_projectionMatrix;
    
    float m_width, m_height;
    float m_left, m_right, m_bottom, m_top;
    float m_nearPlane, m_farPlane;

    mutable bool m_isOrthoDirty;
    mutable bool m_isViewDirty;
    bool m_isActive = true;
};