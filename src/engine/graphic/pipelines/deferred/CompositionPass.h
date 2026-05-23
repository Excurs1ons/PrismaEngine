#pragma once

#include "Export.h"
#include "graphic/LogicalPass.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IDeviceContext.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IShader.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class CompositionPass : public LogicalPass {
public:
    enum class PostProcessEffect : uint32_t {
        None = 0,
        ToneMapping = 1 << 0,
        GammaCorrection = 1 << 1,
        FXAA = 1 << 2,
        SMAA = 1 << 3,
        Bloom = 1 << 4,
        SSR = 1 << 5,
        SSAO = 1 << 6,
        DepthOfField = 1 << 7
    };

    struct RenderStats {
        uint32_t postProcessEffects = 0;
        float renderTime = 0.0f;
    };

    CompositionPass();
    ~CompositionPass() override = default;

    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;
    void Execute(ICommandBuffer* cmd, IRenderDevice* device);

    void SetInputTexture(ITexture* texture) { m_inputTexture = texture; }
    ITexture* GetInputTexture() const { return m_inputTexture; }

    void SetLightingBuffer(IRenderTarget* lightingBuffer) { m_lightingBuffer = lightingBuffer; }
    IRenderTarget* GetLightingBuffer() const { return m_lightingBuffer; }

    void SetAOBuffer(IRenderTarget* aoBuffer) { m_aoBuffer = aoBuffer; }
    IRenderTarget* GetAOBuffer() const { return m_aoBuffer; }

    void SetBloomBuffer(IRenderTarget* bloomBuffer) { m_bloomBuffer = bloomBuffer; }
    IRenderTarget* GetBloomBuffer() const { return m_bloomBuffer; }

    void SetPostProcessEffect(PostProcessEffect effect, bool enable);
    bool IsPostProcessEffectEnabled(PostProcessEffect effect) const;
    void SetToneMappingParams(float exposure, float gamma);
    void SetFXAAParams(float edgeThresholdMin, float edgeThresholdMax);
    void SetSSAOParams(float radius, float bias, float power);

    const RenderStats& GetRenderStats() const { return m_stats; }
    RenderStats& GetRenderStats() { return m_stats; }
    void ResetStats() { m_stats = RenderStats(); }

private:
    bool EnsureDefaultPipeline(IRenderDevice* device);

    ITexture* m_inputTexture;
    IRenderTarget* m_lightingBuffer;
    IRenderTarget* m_aoBuffer;
    IRenderTarget* m_bloomBuffer;

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

    struct ToneMappingParams {
        float exposure = 1.0f;
        float gamma = 2.2f;
    } m_toneMappingParams;

    struct FXAAParams {
        float edgeThresholdMin = 0.0312f;
        float edgeThresholdMax = 0.125f;
    } m_fxaaParams;

    struct SSAOParams {
        float radius = 0.5f;
        float bias = 0.025f;
        float power = 2.0f;
    } m_ssaoParams;

    RenderStats m_stats;

    std::shared_ptr<IShader> m_vertexShader;
    std::shared_ptr<IShader> m_fragmentShader;
    std::shared_ptr<IPipelineState> m_pipelineState;
};

} // namespace Prisma::Graphic
