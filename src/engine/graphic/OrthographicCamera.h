#pragma once

#include "Export.h"
#include "ICamera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Prisma {
namespace Graphic {

/**
 * @brief 2D 正交相机实现
 */
class ENGINE_API OrthographicCamera : public ICamera {
public:
    OrthographicCamera() : OrthographicCamera(-640.0f, 640.0f, -360.0f, 360.0f) {}
    OrthographicCamera(float left, float right, float bottom, float top, float nearPlane = -1.0f, float farPlane = 1.0f);
    virtual ~OrthographicCamera() = default;

    // ========== ICamera 接口实现 ==========

    virtual PrismaMath::mat4 GetViewMatrix() const override { return m_viewMatrix; }
    virtual PrismaMath::mat4 GetProjectionMatrix() const override { return m_projectionMatrix; }
    virtual PrismaMath::mat4 GetViewProjectionMatrix() const override { return m_viewProjectionMatrix; }

    virtual PrismaMath::vec3 GetPosition() const override { return PrismaMath::vec3(m_position, 0.0f); }
    virtual PrismaMath::vec3 GetForward() const override { return PrismaMath::vec3(0.0f, 0.0f, -1.0f); }
    virtual PrismaMath::vec3 GetUp() const override { return PrismaMath::vec3(0.0f, 1.0f, 0.0f); }
    virtual PrismaMath::vec3 GetRight() const override { return PrismaMath::vec3(1.0f, 0.0f, 0.0f); }

    virtual float GetFOV() const override { return 0.0f; } // 正交相机没有 FOV
    virtual float GetNearPlane() const override { return m_near; }
    virtual float GetFarPlane() const override { return m_far; }
    virtual float GetAspectRatio() const override { 
        float width = std::abs(m_right - m_left);
        float height = std::abs(m_top - m_bottom);
        return height > 0.0f ? width / height : 1.0f;
    }

    virtual void SetFOV(float /*fov*/) override {} // 不支持
    virtual void SetNearFarPlanes(float nearPlane, float farPlane) override {
        m_near = nearPlane;
        m_far = farPlane;
        RecalculateMatrices();
    }
    virtual void SetViewport(uint32_t width, uint32_t height) override {
        SetProjection(0.0f, (float)width, 0.0f, (float)height);
    }

    virtual void SetAspectRatio(float aspectRatio) override {
        // 对于正交相机，调整宽高比意味着调整左右边界
        float height = std::abs(m_top - m_bottom);
        float width = height * aspectRatio;
        float centerX = (m_left + m_right) * 0.5f;
        m_left = centerX - width * 0.5f;
        m_right = centerX + width * 0.5f;
        RecalculateMatrices();
    }

    virtual void Update(Timestep ts) override;

    virtual bool IsActive() const override { return m_active; }
    virtual void SetActive(bool active) override { m_active = active; }

    virtual PrismaMath::vec4 GetClearColor() const override { return m_clearColor; }
    virtual void SetClearColor(float r, float g, float b, float a = 1.0f) override {
        m_clearColor = PrismaMath::vec4(r, g, b, a);
    }

    // ========== 正交相机特有方法 ==========

    void SetProjection(float left, float right, float bottom, float top);
    
    void SetViewportSize(float width, float height) {
        SetProjection(-width * 0.5f, width * 0.5f, height * 0.5f, -height * 0.5f);
    }
    
    void SetPosition(const PrismaMath::vec2& position) {
        m_position = position;
        RecalculateMatrices();
    }

    void SetRotation(float rotation) {
        m_rotation = rotation;
        RecalculateMatrices();
    }

    float GetRotation() const { return m_rotation; }

    void SetZoom(float zoom) {
        m_zoom = zoom;
        RecalculateMatrices();
    }

    float GetZoom() const { return m_zoom; }

private:
    void RecalculateMatrices();

    PrismaMath::mat4 m_viewMatrix = PrismaMath::mat4(1.0f);
    PrismaMath::mat4 m_projectionMatrix = PrismaMath::mat4(1.0f);
    PrismaMath::mat4 m_viewProjectionMatrix = PrismaMath::mat4(1.0f);

    PrismaMath::vec2 m_position = {0.0f, 0.0f};
    float m_rotation = 0.0f;
    float m_zoom = 1.0f;

    float m_left, m_right, m_bottom, m_top;
    float m_near, m_far;

    bool m_active = true;
    PrismaMath::vec4 m_clearColor = {0.1f, 0.1f, 0.1f, 1.0f};
};

} // namespace Graphic
} // namespace Prisma
