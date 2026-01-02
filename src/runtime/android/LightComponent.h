#ifndef PRISMA_ENGINE_LIGHT_COMPONENT_H
#define PRISMA_ENGINE_LIGHT_COMPONENT_H

#include "../../engine/Component.h"
#include "math/MathTypes.h"
#include <vector>

namespace PrismaEngine {

/**
 * @brief 光源类型枚举
 */
enum class LightType {
    Directional,  // 方向光（如太阳光）
    Point,        // 点光源（如灯泡）
    Spot          // 聚光灯（如手电筒）
};

/**
 * @brief 光源组件基类
 *
 * 用于为场景添加光照效果。继承自 Component，可以附加到 GameObject 上。
 */
class LightComponent : public Component {
public:
    LightComponent() = default;
    ~LightComponent() override = default;

    /**
     * @brief 获取光源颜色
     */
    Vector3 GetColor() const { return color_; }

    /**
     * @brief 设置光源颜色
     * @param color RGB 颜色值（0.0 - 1.0）
     */
    void SetColor(const Vector3& color) { color_ = color; }

    /**
     * @brief 获取光照强度
     */
    float GetIntensity() const { return intensity_; }

    /**
     * @brief 设置光照强度
     * @param intensity 强度值（默认 1.0）
     */
    void SetIntensity(float intensity) { intensity_ = intensity; }

    /**
     * @brief 获取光源类型
     */
    virtual LightType GetLightType() const = 0;

protected:
    Vector3 color_ = Vector3(1.0f, 1.0f, 1.0f);  // 默认白色
    float intensity_ = 1.0f;
};

/**
 * @brief 方向光组件（如太阳光）
 *
 * 方向光的光线是平行的，没有位置概念，只有方向。
 * 方向由 GameObject 的 rotation 决定。
 */
class DirectionalLight : public LightComponent {
public:
    DirectionalLight() = default;
    ~DirectionalLight() override = default;

    LightType GetLightType() const override { return LightType::Directional; }

    /**
     * @brief 获取光源方向（世界空间）
     *
     * 默认方向是 (0, -1, 0)，即向下照射
     */
    Vector3 GetDirection() const;
};

/**
 * @brief 点光源组件（如灯泡）
 *
 * 点光源从特定位置向所有方向发射光线，强度随距离衰减。
 */
class PointLight : public LightComponent {
public:
    PointLight() = default;
    ~PointLight() override = default;

    LightType GetLightType() const override { return LightType::Point; }

    /**
     * @brief 获取衰减范围
     *
     * 超过这个距离，光照强度为 0
     */
    float GetRange() const { return range_; }

    /**
     * @brief 设置衰减范围
     */
    void SetRange(float range) { range_ = range; }

private:
    float range_ = 10.0f;  // 默认光照范围 10 米
};

} // namespace PrismaEngine

#endif // PRISMA_ENGINE_LIGHT_COMPONENT_H
