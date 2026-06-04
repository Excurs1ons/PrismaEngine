#pragma once

#include "Export.h"
#include "graphic/interfaces/IComputePipeline.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/ISampler.h"
#include <memory>

namespace Prisma::Graphic {

/// TonemappingPass — ACES filmic tone mapping + gamma correction
///
/// Full-screen compute pass that converts HDR scene color to LDR
/// using ACES filmic tone mapping, with configurable exposure and gamma.
class ENGINE_API TonemappingPass {
public:
    TonemappingPass();
    ~TonemappingPass();

    bool Setup(IRenderDevice* device);
    void Execute(ICommandBuffer* cmd, ITexture* input, ITexture* output);
    void Cleanup();

    void SetExposure(float exposure) { m_exposure = exposure; }
    float GetExposure() const { return m_exposure; }
    void SetGamma(float gamma) { m_gamma = gamma; }
    float GetGamma() const { return m_gamma; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }
    bool IsReady() const { return m_ready; }

private:
    bool CreatePipelines();
    bool CreateDescriptorSets();

    IRenderDevice* m_device = nullptr;
    IResourceFactory* m_factory = nullptr;

    float m_exposure = 1.0f;
    float m_gamma = 2.2f;
    bool m_enabled = true;

    std::shared_ptr<IShader> m_tonemapShader;
    std::shared_ptr<IComputePipeline> m_tonemapPipeline;

    std::shared_ptr<IDescriptorSetLayout> m_tonemapDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_tonemapDescSet;

    std::shared_ptr<ISampler> m_linearSampler;

    bool m_ready = false;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Prisma::Graphic
