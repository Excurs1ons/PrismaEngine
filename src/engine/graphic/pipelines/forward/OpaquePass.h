#pragma once

#include "ForwardRenderPassBase.h"
#include "graphic/RenderTypes.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "math/MathTypes.h"
#include <vector>
namespace PrismaEngine::Graphic {

/// @brief 不透明物体逻辑 Pass
/// 前向渲染的主要 Pass，渲染不透明物体
class OpaquePass : public ForwardRenderPass {
public:

    OpaquePass();
    ~OpaquePass() override = default;

    // === IPass 接口实现 ===

    /// @brief 执行 Pass
    /// @param context 执行上下文
    void Execute(const PassExecutionContext& context) override;

    /// @brief 更新 Pass 数据
    /// @param deltaTime 时间增量
    void Update(float deltaTime) override;

    // === 光照设置 ===

    /// @brief 设置光源列表
    /// @param lights 光源数组
    void SetLights(const std::vector<Light>& lights) { m_lights = lights; }

    /// @brief 获取光源列表
    const std::vector<Light>& GetLights() const { return m_lights; }

    /// @brief 设置环境光颜色
    /// @param color 环境光颜色
    void SetAmbientColor(const PrismaMath::vec3& color) { m_ambientColor = color; }

    /// @brief 获取环境光颜色
    const PrismaMath::vec3& GetAmbientColor() const { return m_ambientColor; }

    /// @brief 设置环境光强度
    /// @param intensity 环境光强度
    void SetAmbientIntensity(float intensity) { m_ambientIntensity = intensity; }

    /// @brief 获取环境光强度
    float GetAmbientIntensity() const { return m_ambientIntensity; }

    // === 渲染统计 ===

    /// @brief 获取渲染统计
    struct RenderStats {
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
        uint32_t objects = 0;
    };
    const RenderStats& GetRenderStats() const { return m_stats; }
    RenderStats& GetRenderStats() { return m_stats; }

    /// @brief 重置渲染统计
    void ResetStats() { m_stats = RenderStats(); }

private:
    // 光照数据
    std::vector<Light> m_lights;
    PrismaMath::vec3 m_ambientColor;
    float m_ambientIntensity;

    // 渲染统计
    RenderStats m_stats;
};

} // namespace PrismaEngine::Graphic
