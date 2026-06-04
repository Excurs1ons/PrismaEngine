#pragma once

#include "../../core/Timestep.h"
#include "../../math/MathTypes.h"
#include <memory>

namespace Prisma {
namespace Graphic {

/* 2D 光源类 */
class Light2D {
public:
    enum class Type {
        Point,        // 点光源
        Directional,  // 方向光
        Spot          // 聚光灯
    };

    enum class BlendMode {
        Additive,    // 相加（默认）
        AlphaBlend,  // Alpha 混合
        Multiply,    // 正片叠底
        Subtractive  // 相减
    };

    Light2D(Type type = Type::Point);
    ~Light2D() = default;

    // ========== 类型 ==========

    void SetType(Type type) { m_type = type; }
    Type GetType() const { return m_type; }

    bool IsDirectional() const { return m_type == Type::Directional; }
    bool IsSpot() const { return m_type == Type::Spot; }
    bool IsPoint() const { return m_type == Type::Point; }

    // ========== 位置与方向 ==========

    void SetPosition(const Vector2& position) { m_position = position; }
    const Vector2& GetPosition() const { return m_position; }

    void SetDirection(const Vector2& direction) { m_direction = glm::normalize(direction); }
    const Vector2& GetDirection() const { return m_direction; }

    // ========== 颜色与强度 ==========

    void SetColor(const Vector3& color) { m_color = color; }
    const Vector3& GetColor() const { return m_color; }

    void SetIntensity(float intensity) { m_intensity = glm::max(0.0f, intensity); }
    float GetIntensity() const { return m_intensity; }

    void SetVolumetricIntensity(float intensity) { m_volumetricIntensity = glm::max(0.0f, intensity); }
    float GetVolumetricIntensity() const { return m_volumetricIntensity; }

    // ========== 范围与衰减 ==========

    void SetRadius(float radius) { m_radius = glm::max(0.0f, radius); }
    float GetRadius() const { return m_radius; }

    void SetFalloffCurve(float curve) { m_falloffCurve = glm::max(0.01f, curve); }
    float GetFalloffCurve() const { return m_falloffCurve; }

    // ========== 聚光灯 ==========

    void SetSpotAngle(float angleDegrees);
    float GetSpotAngle() const { return m_spotAngle; }

    // ========== 混合与排序 ==========

    void SetBlendMode(BlendMode mode) { m_blendMode = mode; }
    BlendMode GetBlendMode() const { return m_blendMode; }

    void SetLightOrder(int32_t order) { m_lightOrder = order; }
    int32_t GetLightOrder() const { return m_lightOrder; }

    // ========== 阴影 ==========

    void SetCastShadows(bool cast) { m_castShadows = cast; }
    bool IsCastShadows() const { return m_castShadows; }

    void SetShadowMapSize(uint32_t size) { m_shadowMapSize = size; }
    uint32_t GetShadowMapSize() const { return m_shadowMapSize; }
    void SetShadowSoftness(float softness) { m_shadowSoftness = glm::clamp(softness, 0.0f, 1.0f); }
    float GetShadowSoftness() const { return m_shadowSoftness; }

    // ========== 更新 ==========

    void Update(Timestep ts);

private:
    Type m_type = Type::Point;
    BlendMode m_blendMode = BlendMode::Additive;
    int32_t m_lightOrder = 0;

    Vector2 m_position  = {0.0f, 0.0f};
    Vector2 m_direction = {1.0f, 0.0f};

    Vector3 m_color   = {1.0f, 1.0f, 1.0f};
    float m_intensity = 1.0f;
    float m_volumetricIntensity = 0.0f;

    float m_radius    = 100.0f;
    float m_falloffCurve = 1.0f; // 1.0 是线性衰减
    float m_spotAngle = 45.0f;

    bool m_castShadows = false;
    uint32_t m_shadowMapSize = 256;
    float m_shadowSoftness = 0.5f;
};

}  // namespace Graphic
}  // namespace Prisma