#pragma once

#include "Export.h"
#include "graphic/interfaces/IComputePipeline.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/ISampler.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class IResourceFactory;

class ENGINE_API IBLGenerator {
public:
    IBLGenerator();
    ~IBLGenerator();

    bool Setup(IRenderDevice* device);
    void Cleanup();

    void SetCubemapSource(ITexture* envMap) { m_envMap = envMap; }
    ITexture* GetCubemapSource() const { return m_envMap; }

    ITexture* GenerateIrradianceMap(ICommandBuffer* cmd);
    ITexture* GeneratePrefilterMap(ICommandBuffer* cmd);
    ITexture* GenerateBRDFLUT(ICommandBuffer* cmd);

    ITexture* GetIrradianceMap() const { return m_irradianceMap.get(); }
    ITexture* GetPrefilterMap() const { return m_prefilterMap.get(); }
    ITexture* GetBRDFLUT() const { return m_brdfLUT.get(); }

    void SetSampleDelta(float delta) { m_sampleDelta = delta; }
    float GetSampleDelta() const { return m_sampleDelta; }

    void SetSampleCount(uint32_t count) { m_sampleCount = count; }
    uint32_t GetSampleCount() const { return m_sampleCount; }

    void SetIrradianceResolution(uint32_t res) { m_irradianceRes = res; }
    uint32_t GetIrradianceResolution() const { return m_irradianceRes; }

    void SetPrefilterResolution(uint32_t res) { m_prefilterRes = res; }
    uint32_t GetPrefilterResolution() const { return m_prefilterRes; }

    bool IsReady() const { return m_ready; }

private:
    bool CreatePipelines();
    bool CreateSampler();
    bool CreateDescriptorSets();

    void UpdateIrradianceDescSet();
    void UpdatePrefilterDescSet();
    void UpdateBRDFDescSet();

    std::shared_ptr<ITexture> CreateCubemap(uint32_t resolution,
                                            TextureFormat format,
                                            uint32_t mipLevels);
    std::shared_ptr<ITexture> CreateTexture2D(uint32_t width,
                                              uint32_t height,
                                              TextureFormat format);

    IRenderDevice* m_device = nullptr;
    IResourceFactory* m_factory = nullptr;

    ITexture* m_envMap = nullptr;

    std::shared_ptr<IShader> m_irradianceShader;
    std::shared_ptr<IShader> m_prefilterShader;
    std::shared_ptr<IShader> m_brdfShader;

    std::shared_ptr<IComputePipeline> m_irradiancePipeline;
    std::shared_ptr<IComputePipeline> m_prefilterPipeline;
    std::shared_ptr<IComputePipeline> m_brdfPipeline;

    std::shared_ptr<IDescriptorSet> m_irradianceDescSet;
    std::shared_ptr<IDescriptorSet> m_prefilterDescSet;
    std::shared_ptr<IDescriptorSet> m_brdfDescSet;

    std::shared_ptr<ISampler> m_linearSampler;

    std::shared_ptr<ITexture> m_irradianceMap;
    std::shared_ptr<ITexture> m_prefilterMap;
    std::shared_ptr<ITexture> m_brdfLUT;

    float m_sampleDelta = 0.025f;
    uint32_t m_sampleCount = 256;
    uint32_t m_irradianceRes = 32;
    uint32_t m_prefilterRes = 128;

    bool m_ready = false;
};

}
