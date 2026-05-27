#pragma once

#include "Export.h"
#include "math/MathTypes.h"
#include "Component.h"
#include "graphic/ICamera.h"
#include <array>

namespace Prisma::Graphic {

    /// 投影模式
    enum class ProjectionMode {
        Perspective,
        Orthographic
    };

    class ENGINE_API Camera : public Component, public Prisma::Graphic::ICamera {
    public:
        Camera();
        ~Camera() override;

        // ── Component 接口 ──
        void Initialize() override;
        void Update(Timestep ts) override;
        ComponentId GetComponentId() const override { return GetComponentTypeId<Camera>(); }

        // ── 投影模式 ──
        ProjectionMode GetProjectionMode() const { return m_projectionMode; }
        void SetProjectionMode(ProjectionMode mode);

        void SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane = 0.1f,
                                      float farPlane = 1000.0f);
        void SetOrthographicProjection(float size, float aspectRatio, float nearPlane = 0.1f,
                                        float farPlane = 1000.0f);

        // ── 正交参数 ──
        float GetOrthoSize() const { return m_orthoSize; }
        void SetOrthoSize(float size);

        // ── 移动/旋转/注视 ──
        void MoveWorld(float x, float y, float z);
        void MoveWorld(const Prisma::Vector3& direction);
        void MoveLocal(float forward, float right, float up);
        void Rotate(float pitch, float yaw, float roll);
        void LookAt(const Prisma::Vector3& target);
        void LookAt(float x, float y, float z);

        // ── 数据序列化 ──
        struct Data {
            ProjectionMode projectionMode = ProjectionMode::Perspective;
            float fovDeg = 70.0f;              // 透视模式下的 FOV（度）
            float orthoSize = 5.0f;            // 正交模式下的 size
            float nearPlane = 0.1f;
            float farPlane = 1000.0f;
            std::array<float, 4> clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        };

        Data GetData() const;
        void SetData(const Data& d);

        // ── ICamera 接口 ──
        Prisma::Matrix4x4 GetViewMatrix() const override;
        Prisma::Matrix4x4 GetProjectionMatrix() const override;
        Prisma::Matrix4x4 GetViewProjectionMatrix() const override;
        Prisma::Vector3 GetPosition() const override;
        Prisma::Vector3 GetForward() const override;
        Prisma::Vector3 GetUp() const override;
        Prisma::Vector3 GetRight() const override;
        float GetFOV() const override { return m_fov; }
        void SetFOV(float fov) override;
        float GetNearPlane() const override { return m_nearPlane; }
        float GetFarPlane() const override { return m_farPlane; }
        float GetAspectRatio() const override { return m_aspectRatio; }
        void SetNearFarPlanes(float nearPlane, float farPlane) override;
        void SetAspectRatio(float aspectRatio) override;
        void SetViewport(uint32_t width, uint32_t height) override {
            m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
            m_isProjectionDirty = true;
        }
        bool IsActive() const override { return m_isActive; }
        void SetActive(bool active) override { m_isActive = active; }
        Prisma::Vector4 GetClearColor() const override;
        void SetClearColor(float r, float g, float b, float a = 1.0f) override;

    private:
        void UpdateViewMatrix() const;
        void UpdateVectors() const;
        void MarkViewDirty() const;

        // 投影模式
        ProjectionMode m_projectionMode = ProjectionMode::Perspective;

        // 投影参数
        float m_fov;
        float m_orthoSize;
        float m_aspectRatio;
        float m_nearPlane;
        float m_farPlane;

        // 缓存的矩阵
        mutable Prisma::Matrix4x4 m_viewMatrix;
        mutable Prisma::Matrix4x4 m_projectionMatrix;

        // 缓存的向量（从 Transform 计算得出）
        mutable Prisma::Vector3 m_forward;
        mutable Prisma::Vector3 m_up;
        mutable Prisma::Vector3 m_right;

        // 脏标记
        mutable bool m_isViewDirty;
        mutable bool m_isProjectionDirty;
        bool m_isActive = true;

        // 清除颜色
        PrismaMath::vec4 m_clearColor = PrismaMath::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    };

} // namespace Prisma::Graphic
