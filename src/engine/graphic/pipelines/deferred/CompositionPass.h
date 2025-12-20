#pragma once

#include "graphic/RenderPass.h"
#include <memory>

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Deferred {

// 后处理效果类型
enum class PostProcessEffect : uint32_t {
    None = 0,
    ToneMapping = 1,
    GammaCorrection = 2,
    FXAA = 3,
    SMAA = 4,
    Bloom = 5,
    SSR = 6,  // Screen Space Reflections
    SSAO = 7, // Screen Space Ambient Occlusion
    DepthOfField = 8
};

// 合成通道 - 将光照结果与后处理效果合成最终图像
class CompositionPass : public RenderPass
{
public:
    CompositionPass();
    ~CompositionPass();

    // 渲染通道执行函数
    void Execute(RenderCommandContext* context) override;

    // 设置渲染目标（最终输出的屏幕缓冲区）
    void SetRenderTarget(void* renderTarget) override;

    // 清屏操作
    void ClearRenderTarget(float r, float g, float b, float a) override;

    // 设置视口
    void SetViewport(uint32_t width, uint32_t height) override;

    // 设置光照缓冲区
    void SetLightingBuffer(void* lightingBuffer);

    // 设置环境光遮蔽缓冲区
    void SetAOBuffer(void* aoBuffer);

    // 设置泛光缓冲区
    void SetBloomBuffer(void* bloomBuffer);

    // 启用/禁用后处理效果
    void SetPostProcessEffect(PostProcessEffect effect, bool enable);

    // 检查后处理效果是否启用
    bool IsPostProcessEffectEnabled(PostProcessEffect effect) const;

    // 设置色调映射参数
    void SetToneMappingParams(float exposure, float gamma);

    // 设置FXAA参数
    void SetFXAAParams(float edgeThresholdMin, float edgeThresholdMax);

    // 设置SSAO参数
    void SetSSAOParams(float radius, float bias, float power);

private:
    // 渲染目标
    void* m_renderTarget = nullptr;

    // 输入缓冲区
    void* m_lightingBuffer = nullptr;
    void* m_aoBuffer = nullptr;
    void* m_bloomBuffer = nullptr;

    // 视口尺寸
    uint32_t m_width = 0;
    uint32_t m_height = 0;

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

    // FXAA参数
    struct FXAAParams {
        float edgeThresholdMin = 0.0312f;
        float edgeThresholdMax = 0.125f;
    } m_fxaaParams;

    // SSAO参数
    struct SSAOParams {
        float radius = 0.5f;
        float bias = 0.025f;
        float power = 2.0f;
    } m_ssaoParams;

    // 渲染统计
    struct CompositionPassStats {
        uint32_t postProcessEffects = 0;
        float renderTime = 0.0f;
    } m_stats;

    // 渲染全屏四边形
    void RenderFullScreenQuad(RenderCommandContext* context);

    // 应用色调映射
    void ApplyToneMapping(RenderCommandContext* context);

    // 应用FXAA抗锯齿
    void ApplyFXAA(RenderCommandContext* context);

    // 应用SMAA抗锯齿
    void ApplySMAA(RenderCommandContext* context);

    // 应用泛光效果
    void ApplyBloom(RenderCommandContext* context);

    // 应用SSR效果
    void ApplySSR(RenderCommandContext* context);

    // 应用SSAO效果
    void ApplySSAO(RenderCommandContext* context);

    // 应用景深效果
    void ApplyDepthOfField(RenderCommandContext* context);
};

} // namespace Deferred
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine