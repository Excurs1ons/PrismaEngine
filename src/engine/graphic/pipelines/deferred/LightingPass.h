#pragma once

#include "graphic/LogicalPass.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IDeviceContext.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/IGBuffer.h"
#include "graphic/interfaces/ITexture.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>

namespace PrismaEngine::Graphic {

/// @brief 光照逻辑 Pass
/// 使用 G-Buffer 计算场景光照
class LightingPass : public LogicalPass {
public:
    // 光源类型
    enum class LightType : uint32_t {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    // 光源结构
    struct Light {
        LightType type = LightType::Point;
        PrismaMath::vec3 position = {0.0f, 0.0f, 0.0f};
        PrismaMath::vec3 direction = {0.0f, -1.0f, 0.0f};
        PrismaMath::vec3 color = {1.0f, 1.0f, 1.0f};
        float intensity = 1.0f;
        float range = 10.0f;
        float innerCone = 0.5f;
        float outerCone = 1.0f;
        bool castShadows = false;
        uint32_t shadowMapIndex = 0xFFFFFFFF;
        PrismaMath::mat4 shadowMatrix = PrismaMath::mat4(1.0f);
    };

    // 渲染统计
    struct RenderStats {
        uint32_t lightsRendered = 0;
        uint32_t shadowCastingLights = 0;
    };

public:
    LightingPass();
    ~LightingPass() override = default;

    // === IPass 接口实现 ===

    /// @brief 执行 Pass
    /// @param context 执行上下文
    void Execute(const PassExecutionContext& context) override;

    /// @brief 更新 Pass 数据
    /// @param deltaTime 时间增量
    void Update(float deltaTime) override;

    // === G-Buffer 设置 ===

    /// @brief 设置 G-Buffer
    /// @param gBuffer G-Buffer 接口指针
    void SetGBuffer(IGBuffer* gBuffer) { m_gBuffer = gBuffer; }

    /// @brief 获取 G-Buffer
    IGBuffer* GetGBuffer() const { return m_gBuffer; }

    // === 光照设置 ===

    /// @brief 设置环境光
    void SetAmbientLight(const PrismaMath::vec3& ambient) { m_ambientLight = ambient; }
    const PrismaMath::vec3& GetAmbientLight() const { return m_ambientLight; }

    /// @brief 设置光源列表
    void SetLights(const std::vector<Light>& lights) { m_lights = lights; }
    const std::vector<Light>& GetLights() const { return m_lights; }

    /// @brief 添加光源
    void AddLight(const Light& light) { m_lights.push_back(light); }

    /// @brief 清除所有光源
    void ClearLights() { m_lights.clear(); }

    // === IBL 设置 ===

    /// @brief 设置是否启用 IBL
    void SetIBL(bool enable) { m_iblEnabled = enable; }
    bool GetIBL() const { return m_iblEnabled; }

    /// @brief 设置 IBL 纹理
    void SetIBLTextures(ITexture* irradianceMap, ITexture* prefilterMap, ITexture* brdfLUT);

    // === 渲染统计 ===

    /// @brief 获取渲染统计
    const RenderStats& GetRenderStats() const { return m_stats; }
    RenderStats& GetRenderStats() { return m_stats; }

    /// @brief 重置渲染统计
    void ResetStats() { m_stats = RenderStats(); }

private:
    // G-Buffer
    IGBuffer* m_gBuffer;

    // 环境光
    PrismaMath::vec3 m_ambientLight;

    // 光源列表
    std::vector<Light> m_lights;

    // IBL 设置
    bool m_iblEnabled;
    ITexture* m_irradianceMap;
    ITexture* m_prefilterMap;
    ITexture* m_brdfLUT;

    // 渲染统计
    RenderStats m_stats;
};

} // namespace PrismaEngine::Graphic
