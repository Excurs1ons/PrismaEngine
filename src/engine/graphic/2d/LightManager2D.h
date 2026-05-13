#pragma once

#include "Light2D.h"
#include <vector>
#include <memory>
#include <mutex>
#include "../../Export.h"

namespace Prisma::Graphic {

/**
 * @brief 2D 光源管理器
 * 管理场景中所有的 2D 光源
 */
class ENGINE_API LightManager2D {
public:
    static LightManager2D& Get();

    /**
     * @brief 创建一个新的 2D 光源
     * @return 光源句柄 (Index + 1)
     */
    uint32_t CreateLight(Light2D::Type type = Light2D::Type::Point);

    /**
     * @brief 销毁一个 2D 光源
     */
    void DestroyLight(uint32_t handle);

    /**
     * @brief 获取光源对象
     */
    Light2D* GetLight(uint32_t handle);

    /**
     * @brief 获取所有活跃的光源
     */
    const std::vector<std::shared_ptr<Light2D>>& GetLights() const { return m_lights; }

    // ========== 环境光 ==========
    // 环境色作为光照纹理的清除色，控制场景基础照明级别。
    // (1,1,1) = 完全照亮（无光照区域 sprite 正常可见）
    // (0,0,0) = 无环境光（只有光源区域可见）
    // 中间值 = 环境暗度

    void SetAmbientColor(const Vector3& color) { m_ambientColor = color; }
    const Vector3& GetAmbientColor() const { return m_ambientColor; }

private:
    LightManager2D() = default;
    ~LightManager2D() = default;

    std::vector<std::shared_ptr<Light2D>> m_lights;
    std::mutex m_mutex;
    Vector3 m_ambientColor = {1.0f, 1.0f, 1.0f}; // 默认完全照亮
};

} // namespace Prisma::Graphic
