#include "IBLGenerator.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "app/Engine.h"
#include "Logger.h"
#include <cmath>

namespace Prisma::Graphic {

IBLGenerator::IBLGenerator() = default;

IBLGenerator::~IBLGenerator() {
    Cleanup();
}

bool IBLGenerator::Setup(IRenderDevice* device) {
    if (!device) return false;
    m_device = device;
    m_factory = device->GetResourceFactory();
    if (!m_factory) return false;

    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rm) return false;

    m_irradianceShader = rm->LoadShaderSync("assets/shaders/pbr_ibl_irradiance.comp.spv");
    m_prefilterShader = rm->LoadShaderSync("assets/shaders/pbr_ibl_prefilter.comp.spv");
    m_brdfShader = rm->LoadShaderSync("assets/shaders/pbr_ibl_brdf.comp.spv");

    if (!m_irradianceShader || !m_prefilterShader || !m_brdfShader) {
        LOG_WARN("IBLGenerator", "IBL shaders not found. "
                  "Place pbr_ibl_irradiance/prefilter/brdf.comp.spv in assets/shaders/");
        return false;
    }

    if (!CreatePipelines()) {
        LOG_ERROR("IBLGenerator", "Failed to create compute pipelines");
        return false;
    }

    if (!CreateDescriptorSets()) {
        LOG_ERROR("IBLGenerator", "Failed to create descriptor sets");
        return false;
    }

    if (!CreateSampler()) {
        LOG_ERROR("IBLGenerator", "Failed to create sampler");
        return false;
    }

    m_ready = true;
    LOG_INFO("IBLGenerator", "IBL Generator initialized successfully");
    return true;
}

void IBLGenerator::Cleanup() {
    m_irradianceShader.reset();
    m_prefilterShader.reset();
    m_brdfShader.reset();

    m_irradiancePipeline.reset();
    m_prefilterPipeline.reset();
    m_brdfPipeline.reset();

    m_irradianceDescSet.reset();
    m_prefilterDescSet.reset();
    m_brdfDescSet.reset();

    m_linearSampler.reset();

    m_irradianceMap.reset();
    m_prefilterMap.reset();
    m_brdfLUT.reset();

    m_envMap = nullptr;
    m_ready = false;
    m_device = nullptr;
    m_factory = nullptr;
}

ITexture* IBLGenerator::GenerateIrradianceMap(ICommandBuffer* cmd) {
    if (!m_ready || !cmd || !m_envMap) return nullptr;

    const uint32_t res = m_irradianceRes;

    m_irradianceMap = CreateCubemap(res, TextureFormat::RGBA16_Float, 1);
    if (!m_irradianceMap) {
        LOG_ERROR("IBLGenerator", "Failed to create irradiance cubemap");
        return nullptr;
    }

    UpdateIrradianceDescSet();

    std::vector<ImageBarrier> barriers;
    barriers.push_back({m_envMap, ResourceState::Undefined, ResourceState::ShaderRead});
    barriers.push_back({m_irradianceMap.get(), ResourceState::Undefined, ResourceState::UnorderedAccess});
    cmd->PipelineBarrier(barriers);

    cmd->SetComputePipeline(m_irradiancePipeline.get());
    cmd->BindDescriptorSet(0, m_irradianceDescSet.get());

    struct {
        float sampleDelta;
        float padding[3];
    } pc;
    pc.sampleDelta = m_sampleDelta;
    cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

    uint32_t gw = (res + 15) / 16;
    uint32_t gh = (res + 15) / 16;
    cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 6);

    return m_irradianceMap.get();
}

ITexture* IBLGenerator::GeneratePrefilterMap(ICommandBuffer* cmd) {
    if (!m_ready || !cmd || !m_envMap) return nullptr;

    const uint32_t res = m_prefilterRes;
    const uint32_t mipCount = static_cast<uint32_t>(
        std::floor(std::log2(static_cast<float>(res)))) + 1;

    m_prefilterMap = CreateCubemap(res, TextureFormat::RGBA16_Float, mipCount);
    if (!m_prefilterMap) {
        LOG_ERROR("IBLGenerator", "Failed to create prefilter cubemap");
        return nullptr;
    }

    std::vector<ImageBarrier> inputBarrier;
    inputBarrier.push_back({m_envMap, ResourceState::Undefined, ResourceState::ShaderRead});
    cmd->PipelineBarrier(inputBarrier);

    for (uint32_t mip = 0; mip < mipCount; ++mip) {
        uint32_t mipWidth = std::max(res >> mip, 1u);
        uint32_t mipHeight = std::max(res >> mip, 1u);

        std::vector<ImageBarrier> mipBarrier;
        mipBarrier.push_back({m_prefilterMap.get(), ResourceState::Undefined, ResourceState::UnorderedAccess, mip, 0});
        cmd->PipelineBarrier(mipBarrier);

        UpdatePrefilterDescSet();

        cmd->SetComputePipeline(m_prefilterPipeline.get());
        cmd->BindDescriptorSet(0, m_prefilterDescSet.get());

        struct {
            float roughness;
            uint32_t sampleCount;
            float padding[2];
        } pc;
        pc.roughness = static_cast<float>(mip) / static_cast<float>(std::max(mipCount - 1, 1u));
        pc.sampleCount = m_sampleCount;
        cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

        uint32_t gw = (mipWidth + 15) / 16;
        uint32_t gh = (mipHeight + 15) / 16;
        cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 6);
    }

    return m_prefilterMap.get();
}

ITexture* IBLGenerator::GenerateBRDFLUT(ICommandBuffer* cmd) {
    if (!m_ready || !cmd) return nullptr;

    const uint32_t res = 512;

    m_brdfLUT = CreateTexture2D(res, res, TextureFormat::RGBA16_Float);
    if (!m_brdfLUT) {
        LOG_ERROR("IBLGenerator", "Failed to create BRDF LUT texture");
        return nullptr;
    }

    UpdateBRDFDescSet();

    std::vector<ImageBarrier> barriers;
    barriers.push_back({m_brdfLUT.get(), ResourceState::Undefined, ResourceState::UnorderedAccess});
    cmd->PipelineBarrier(barriers);

    cmd->SetComputePipeline(m_brdfPipeline.get());
    cmd->BindDescriptorSet(0, m_brdfDescSet.get());

    uint32_t gw = (res + 15) / 16;
    uint32_t gh = (res + 15) / 16;
    cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);

    return m_brdfLUT.get();
}

bool IBLGenerator::CreatePipelines() {
    if (!m_factory || !m_device) return false;

    m_irradiancePipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_irradiancePipeline->SetShader(m_irradianceShader);
    m_irradiancePipeline->SetPushConstantRange(16);
    if (!m_irradiancePipeline->Create(m_device)) {
        LOG_ERROR("IBLGenerator", "Failed to create irradiance compute pipeline");
        return false;
    }

    m_prefilterPipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_prefilterPipeline->SetShader(m_prefilterShader);
    m_prefilterPipeline->SetPushConstantRange(16);
    if (!m_prefilterPipeline->Create(m_device)) {
        LOG_ERROR("IBLGenerator", "Failed to create prefilter compute pipeline");
        return false;
    }

    m_brdfPipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_brdfPipeline->SetShader(m_brdfShader);
    m_brdfPipeline->SetPushConstantRange(0);
    if (!m_brdfPipeline->Create(m_device)) {
        LOG_ERROR("IBLGenerator", "Failed to create BRDF compute pipeline");
        return false;
    }

    return true;
}

bool IBLGenerator::CreateSampler() {
    if (!m_factory) return false;

    SamplerDesc samplerDesc;
    samplerDesc.filter = TextureFilter::Linear;
    samplerDesc.addressU = TextureAddressMode::Clamp;
    samplerDesc.addressV = TextureAddressMode::Clamp;
    samplerDesc.addressW = TextureAddressMode::Clamp;
    m_linearSampler.reset(m_factory->CreateSamplerImpl(samplerDesc).release());
    return m_linearSampler != nullptr;
}

bool IBLGenerator::CreateDescriptorSets() {
    if (!m_factory) return false;

    {
        auto layouts = m_irradiancePipeline->GetDescriptorSetLayouts();
        if (layouts.empty()) return false;
        m_irradianceDescSet = m_factory->CreateDescriptorSet(layouts[0].get());
        if (!m_irradianceDescSet) return false;
    }

    {
        auto layouts = m_prefilterPipeline->GetDescriptorSetLayouts();
        if (layouts.empty()) return false;
        m_prefilterDescSet = m_factory->CreateDescriptorSet(layouts[0].get());
        if (!m_prefilterDescSet) return false;
    }

    {
        auto layouts = m_brdfPipeline->GetDescriptorSetLayouts();
        if (layouts.empty()) return false;
        m_brdfDescSet = m_factory->CreateDescriptorSet(layouts[0].get());
        if (!m_brdfDescSet) return false;
    }

    return true;
}

void IBLGenerator::UpdateIrradianceDescSet() {
    if (!m_irradianceDescSet || !m_envMap || !m_irradianceMap) return;

    m_irradianceDescSet->BindTexture(0, m_envMap, m_linearSampler.get());
    m_irradianceDescSet->BindStorageImage(1, m_irradianceMap.get());
    m_irradianceDescSet->Update();
}

void IBLGenerator::UpdatePrefilterDescSet() {
    if (!m_prefilterDescSet || !m_envMap || !m_prefilterMap) return;

    m_prefilterDescSet->BindTexture(0, m_envMap, m_linearSampler.get());
    m_prefilterDescSet->BindStorageImage(1, m_prefilterMap.get());
    m_prefilterDescSet->Update();
}

void IBLGenerator::UpdateBRDFDescSet() {
    if (!m_brdfDescSet || !m_brdfLUT) return;

    m_brdfDescSet->BindStorageImage(0, m_brdfLUT.get());
    m_brdfDescSet->Update();
}

std::shared_ptr<ITexture> IBLGenerator::CreateCubemap(uint32_t resolution,
                                                      TextureFormat format,
                                                      uint32_t mipLevels) {
    if (!m_factory) return nullptr;

    TextureDesc desc;
    desc.type = TextureType::TextureCube;
    desc.format = format;
    desc.width = resolution;
    desc.height = resolution;
    desc.depth = 1;
    desc.mipLevels = mipLevels;
    desc.arraySize = 6;
    desc.allowRenderTarget = false;
    desc.allowUnorderedAccess = true;
    desc.allowShaderResource = true;

    return m_factory->CreateTextureImpl(desc);
}

std::shared_ptr<ITexture> IBLGenerator::CreateTexture2D(uint32_t width,
                                                        uint32_t height,
                                                        TextureFormat format) {
    if (!m_factory) return nullptr;

    TextureDesc desc;
    desc.type = TextureType::Texture2D;
    desc.format = format;
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    desc.mipLevels = 1;
    desc.arraySize = 1;
    desc.allowRenderTarget = false;
    desc.allowUnorderedAccess = true;
    desc.allowShaderResource = true;

    return m_factory->CreateTextureImpl(desc);
}

}
