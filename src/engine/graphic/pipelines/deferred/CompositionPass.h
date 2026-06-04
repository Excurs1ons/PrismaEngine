#pragma once

#include "Export.h"
#include "graphic/LogicalPass.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IDeviceContext.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/ISampler.h"
#include "graphic/interfaces/ITexture.h"
#include "math/MathTypes.h"
#include "SSRPass.h"
#include "TonemappingPass.h"
#include "FogPass.h"
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
        DepthOfField = 1 << 7,
        Fog = 1 << 8
    };

    struct RenderStats {
        uint32_t postProcessEffects = 0;
        float renderTime = 0.0f;
    };

    CompositionPass();
    ~CompositionPass() override = default;

    bool Setup(IRenderDevice* device);
    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;
    void Execute(ICommandBuffer* cmd, IRenderDevice* device);

    // Input / output buffers
    void SetInputTexture(ITexture* texture) { m_inputTexture = texture; }
    ITexture* GetInputTexture() const { return m_inputTexture; }

    void SetLightingBuffer(IRenderTarget* lightingBuffer) { m_lightingBuffer = lightingBuffer; }
    IRenderTarget* GetLightingBuffer() const { return m_lightingBuffer; }

    void SetAOBuffer(IRenderTarget* aoBuffer) { m_aoBuffer = aoBuffer; }
    IRenderTarget* GetAOBuffer() const { return m_aoBuffer; }

    void SetBloomBuffer(IRenderTarget* bloomBuffer) { m_bloomBuffer = bloomBuffer; }
    IRenderTarget* GetBloomBuffer() const { return m_bloomBuffer; }

    void SetOutputTexture(ITexture* texture) { m_outputTexture = texture; }
    ITexture* GetOutputTexture() const { return m_outputTexture; }

    // G-buffer textures (for SSR, Fog)
    void SetGBuffers(ITexture* color, ITexture* normal, ITexture* depth, ITexture* position) {
        m_gbColor = color;
        m_gbNormal = normal;
        m_gbDepth = depth;
        m_gbPosition = position;
    }

    // Post-process effect configuration
    void SetPostProcessEffect(PostProcessEffect effect, bool enable);
    bool IsPostProcessEffectEnabled(PostProcessEffect effect) const;
    void SetToneMappingParams(float exposure, float gamma);
    void SetFXAAParams(float edgeThresholdMin, float edgeThresholdMax);
    void SetSSAOParams(float radius, float bias, float power);

    const RenderStats& GetRenderStats() const { return m_stats; }
    RenderStats& GetRenderStats() { return m_stats; }
    void ResetStats() { m_stats = RenderStats(); }

    // Sub-pass access
    SSRPass* GetSSRPass() { return m_ssrPass.get(); }
    TonemappingPass* GetTonemappingPass() { return m_tonemapPass.get(); }
    FogPass* GetFogPass() { return m_fogPass.get(); }

private:
    bool EnsureDefaultPipeline(IRenderDevice* device);

    // Core textures
    ITexture* m_inputTexture;
    ITexture* m_outputTexture;
    IRenderTarget* m_lightingBuffer;
    IRenderTarget* m_aoBuffer;
    IRenderTarget* m_bloomBuffer;

    // G-buffer textures (owned by G-Buffer pass, borrowed here)
    ITexture* m_gbColor;
    ITexture* m_gbNormal;
    ITexture* m_gbDepth;
    ITexture* m_gbPosition;

    struct PostProcessSettings {
        bool toneMapping = true;
        bool gammaCorrection = true;
        bool fxaa = false;
        bool smaa = false;
        bool bloom = false;
        bool ssr = false;
        bool ssao = false;
        bool depthOfField = false;
        bool fog = false;
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

    // Sub-passes
    std::unique_ptr<SSRPass> m_ssrPass;
    std::unique_ptr<TonemappingPass> m_tonemapPass;
    std::unique_ptr<FogPass> m_fogPass;

    // Default fullscreen pipeline (existing)
    std::shared_ptr<IShader> m_vertexShader;
    std::shared_ptr<IShader> m_fragmentShader;
    std::shared_ptr<IPipelineState> m_pipelineState;

    // Composition descriptor set (for final composite)
    std::shared_ptr<IDescriptorSetLayout> m_compositeDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_compositeDescSet;
    std::shared_ptr<ISampler> m_linearSampler;
};

} // namespace Prisma::Graphic
