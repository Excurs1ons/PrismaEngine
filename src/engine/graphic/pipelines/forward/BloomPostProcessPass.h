#pragma once

#include "interfaces/RenderTypes.h"
#include "graphic/interfaces/IComputePipeline.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/ISampler.h"
#include "graphic/interfaces/IRenderTarget.h"
#include <memory>

namespace Prisma::Graphic {

class ENGINE_API BloomPostProcessPass {
public:
    BloomPostProcessPass();
    ~BloomPostProcessPass();

    bool Setup(IRenderDevice* device);
    void Execute(ICommandBuffer* cmd, ITexture* source, ITextureRenderTarget* target);
    void Cleanup();

    void SetThreshold(float threshold) { m_threshold = threshold; }
    float GetThreshold() const { return m_threshold; }
    void SetBlurRadius(float radius) { m_blurRadius = radius; }
    float GetBlurRadius() const { return m_blurRadius; }
    void SetBlurIterations(int iterations) { m_blurIterations = iterations; }
    int GetBlurIterations() const { return m_blurIterations; }
    bool IsReady() const { return m_ready; }

private:
    bool CreateTextures(uint32_t width, uint32_t height);
    bool CreatePipelines();
    bool CreateDescriptorSets(uint32_t width, uint32_t height);

    IRenderDevice* m_device = nullptr;
    IResourceFactory* m_factory = nullptr;

    float m_threshold = 1.0f;
    float m_blurRadius = 1.0f;
    int m_blurIterations = 3;

    std::shared_ptr<IShader> m_prefilterShader;
    std::shared_ptr<IShader> m_blurShader;
    std::shared_ptr<IShader> m_compositeShader;

    std::shared_ptr<IComputePipeline> m_prefilterPipeline;
    std::shared_ptr<IComputePipeline> m_blurPipeline;
    std::shared_ptr<IComputePipeline> m_compositePipeline;

    std::shared_ptr<ITexture> m_brightTexture;
    std::shared_ptr<ITexture> m_tempTextureA;
    std::shared_ptr<ITexture> m_tempTextureB;

    std::shared_ptr<IDescriptorSetLayout> m_prefilterDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_prefilterDescSet;
    std::shared_ptr<IDescriptorSetLayout> m_blurDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_blurDescSet;
    std::shared_ptr<IDescriptorSetLayout> m_compositeDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_compositeDescSet;

    std::shared_ptr<ISampler> m_linearSampler;

    bool m_ready = false;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Prisma::Graphic
