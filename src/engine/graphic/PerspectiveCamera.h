#pragma once

#include "Export.h"
#include "ICamera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Prisma {
namespace Graphic {

/**
 * @brief 3D 透视相机实现（独立于 ECS Component 系统）
 */
class ENGINE_API PerspectiveCamera : public ICamera {
public:
    PerspectiveCamera();
    PerspectiveCamera(float fov, float aspectRatio, float nearPlane = 0.1f, float farPlane = 1000.0f);
    virtual ~PerspectiveCamera() = default;

    // ========== ICamera 接口实现 ==========

    PrismaMath::mat4 GetViewMatrix() const override;
    PrismaMath::mat4 GetProjectionMatrix() const override;
    PrismaMath::mat4 GetViewProjectionMatrix() const override;

    PrismaMath::vec3 GetPosition() const override { return m_position; }
    PrismaMath::vec3 GetForward() const override { return m_forward; }
    PrismaMath::vec3 GetUp() const override { return m_up; }
    PrismaMath::vec3 GetRight() const override { return m_right; }

    float GetFOV() const override { return m_fov; }
    float GetNearPlane() const override { return m_nearPlane; }
    float GetFarPlane() const override { return m_farPlane; }
    float GetAspectRatio() const override { return m_aspectRatio; }

    void SetFOV(float fov) override;
    void SetNearFarPlanes(float nearPlane, float farPlane) override;
    void SetAspectRatio(float aspectRatio) override;
    void SetViewport(uint32_t width, uint32_t height) override;

    void Update(Timestep ts) override {}

    bool IsActive() const override { return m_active; }
    void SetActive(bool active) override { m_active = active; }

    PrismaMath::vec4 GetClearColor() const override { return m_clearColor; }
    void SetClearColor(float r, float g, float b, float a = 1.0f) override;

    // ========== 透视相机特有方法 ==========

    /// 设置相机位置
    void SetPosition(const PrismaMath::vec3& pos);

    /// 设置朝向（四元数）
    void SetRotation(const glm::quat& rotation);

    /// 看向目标点
    void LookAt(const PrismaMath::vec3& target);
    void LookAt(float x, float y, float z);

    /// 移动相机（相对于世界坐标系）
    void MoveWorld(const PrismaMath::vec3& delta);

    /// 从位置/目标/上向量计算朝向
    void SetLookAt(const PrismaMath::vec3& position, const PrismaMath::vec3& target, const PrismaMath::vec3& up = PrismaMath::vec3(0.0f, 1.0f, 0.0f));

private:
    void RecalculateMatrices() const;
    void RecalculateVectors() const;

    // 相机参数
    PrismaMath::vec3 m_position = PrismaMath::vec3(0.0f);
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    float m_fov;
    float m_aspectRatio;
    float m_nearPlane;
    float m_farPlane;

    // 缓存向量
    mutable PrismaMath::vec3 m_forward = PrismaMath::vec3(0.0f, 0.0f, -1.0f);
    mutable PrismaMath::vec3 m_up = PrismaMath::vec3(0.0f, 1.0f, 0.0f);
    mutable PrismaMath::vec3 m_right = PrismaMath::vec3(1.0f, 0.0f, 0.0f);

    // 缓存矩阵
    mutable PrismaMath::mat4 m_viewMatrix = PrismaMath::mat4(1.0f);
    mutable PrismaMath::mat4 m_projectionMatrix = PrismaMath::mat4(1.0f);

    // 脏标记
    mutable bool m_isViewDirty = true;
    mutable bool m_isProjectionDirty = true;

    bool m_active = true;
    PrismaMath::vec4 m_clearColor = PrismaMath::vec4(0.0f, 0.0f, 0.0f, 1.0f);
};

} // namespace Graphic
} // namespace Prisma
