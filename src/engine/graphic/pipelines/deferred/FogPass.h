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

/// FogPass — depth-based and height-based exponential fog
///
/// Combines depth fog (exp(-density * depth)) and
/// height fog (exp(-density * abs(height - fogHeight)))
/// applied as a post-process compute pass.
class ENGINE_API FogPass {
public:
    FogPass();
    ~FogPass();

    bool Setup(IRenderDevice* device);
    void Execute(ICommandBuffer* cmd, ITexture* sceneColor, ITexture* gbDepth,
                 ITexture* gbPosition, ITexture* output);
    void Cleanup();

    void SetDensity(float density) { m_density = density; }
    float GetDensity() const { return m_density; }
    void SetHeight(float height) { m_fogHeight = height; }
    float GetHeight() const { return m_fogHeight; }
    void SetFogColor(float r, float g, float b) { m_fogColor[0] = r; m_fogColor[1] = g; m_fogColor[2] = b; }
    const float* GetFogColor() const { return m_fogColor; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }
    bool IsReady() const { return m_ready; }

private:
    bool CreatePipelines();
    bool CreateDescriptorSets();

    IRenderDevice* m_device = nullptr;
    IResourceFactory* m_factory = nullptr;

    float m_density = 0.05f;
    float m_fogHeight = 0.0f;
    float m_fogColor[3] = {0.5f, 0.6f, 0.7f};
    bool m_enabled = true;

    std::shared_ptr<IShader> m_fogShader;
    std::shared_ptr<IComputePipeline> m_fogPipeline;

    std::shared_ptr<IDescriptorSetLayout> m_fogDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_fogDescSet;

    std::shared_ptr<ISampler> m_linearSampler;

    bool m_ready = false;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Prisma::Graphic
