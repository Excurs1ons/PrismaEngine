#pragma once

#include "graphic/LogicalPass.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IDeviceContext.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>

namespace PrismaEngine::Graphic {

/// @brief 合成逻辑 Pass
/// 将光照结果与后处理效果合成最终图像
class CompositionPass : public LogicalPass {
public:
    // 后处理效果类型
    enum class PostProcessEffect : uint32_t {
        None = 0,
        ToneMapping = 1 << 0,
        GammaCorrection = 1 << 1,
        FXAA = 1 << 2,
        SMAA = 1 << 3,
        Bloom = 1 << 4,
        SSR = 1 << 5,   // Screen Space Reflections
        SSAO = 1 << 6,  // Screen Space Ambient Occlusion
        DepthOfField = 1 << 7
    };

    // 渲染统计
    struct RenderStats {
        uint32_t postProcessEffects = 0;
        float renderTime = 0.0f;
    };

public:
    CompositionPass();
    ~CompositionPass() override = default;

    // === IPass 接口实现 ===

    /// @brief 执行 Pass
    /// @param context 执行上下文
    void Execute(const PassExecutionContext& context) override;

    /// @brief 更新 Pass 数据
    /// @param deltaTime 时间增量
    void Update(float deltaTime) override;

    // === 输入缓冲区设置 ===

    /// @brief 设置光照缓冲区
    void SetLightingBuffer(IRenderTarget* lightingBuffer) { m_lightingBuffer = lightingBuffer; }
    IRenderTarget* GetLightingBuffer() const { return m_lightingBuffer; }

    /// @brief 设置 AO 缓冲区
    void SetAOBuffer(IRenderTarget* aoBuffer) { m_aoBuffer = aoBuffer; }
    IRenderTarget* GetAOBuffer() const { return m_aoBuffer; }

    /// @brief 设置泛光缓冲区
    void SetBloomBuffer(IRenderTarget* bloomBuffer) { m_bloomBuffer = bloomBuffer; }
    IRenderTarget* GetBloomBuffer() const { return m_bloomBuffer; }

    // === 后处理效果设置 ===

    /// @brief 设置后处理效果
    void SetPostProcessEffect(PostProcessEffect effect, bool enable);

    /// @brief 检查后处理效果是否启用
    bool IsPostProcessEffectEnabled(PostProcessEffect effect) const;

    /// @brief 设置色调映射参数
    void SetToneMappingParams(float exposure, float gamma);

    /// @brief 设置 FXAA 参数
    void SetFXAAParams(float edgeThresholdMin, float edgeThresholdMax);

    /// @brief 设置 SSAO 参数
    void SetSSAOParams(float radius, float bias, float power);

    // === 渲染统计 ===

    /// @brief 获取渲染统计
    const RenderStats& GetRenderStats() const { return m_stats; }
    RenderStats& GetRenderStats() { return m_stats; }

    /// @brief 重置渲染统计
    void ResetStats() { m_stats = RenderStats(); }

private:
    // 输入缓冲区
    IRenderTarget* m_lightingBuffer;
    IRenderTarget* m_aoBuffer;
    IRenderTarget* m_bloomBuffer;

    // 后处理效果状态
    struct PostProcessSettings {
        bool toneMapping = true;
        bool gammaCorrection = true;
        bool fxaa = false;
        bool smaa = false;
        bool bloom = false;
        bool ssr = false;
        bool ssao = false;
        bool depthOfField = false;
    } m_postProcessSettings;

    // 色调映射参数
    struct ToneMappingParams {
        float exposure = 1.0f;
        float gamma = 2.2f;
    } m_toneMappingParams;

    // FXAA 参数
    struct FXAAParams {
        float edgeThresholdMin = 0.0312f;
        float edgeThresholdMax = 0.125f;
    } m_fxaaParams;

    // SSAO 参数
    struct SSAOParams {
        float radius = 0.5f;
        float bias = 0.025f;
        float power = 2.0f;
    } m_ssaoParams;

    // 渲染统计
    RenderStats m_stats;
};

} // namespace PrismaEngine::Graphic
