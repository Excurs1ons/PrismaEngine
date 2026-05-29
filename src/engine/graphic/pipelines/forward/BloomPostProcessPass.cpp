#include "BloomPostProcessPass.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "app/Engine.h"
#include "Logger.h"

namespace Prisma::Graphic {

BloomPostProcessPass::BloomPostProcessPass() = default;

BloomPostProcessPass::~BloomPostProcessPass() {
    Cleanup();
}

bool BloomPostProcessPass::Setup(IRenderDevice* device) {
    if (!device) return false;
    m_device = device;
    m_factory = device->GetResourceFactory();
    if (!m_factory) return false;

    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rm) return false;

    m_prefilterShader = rm->LoadShaderSync("assets/shaders/bloom_prefilter.comp.spv");
    m_blurShader = rm->LoadShaderSync("assets/shaders/bloom_blur.comp.spv");
    m_compositeShader = rm->LoadShaderSync("assets/shaders/bloom_composite.comp.spv");

    if (!m_prefilterShader || !m_blurShader || !m_compositeShader) {
        LOG_WARN("BloomPostProcessPass", "Bloom shaders not found, bloom disabled. "
                  "Place bloom_prefilter/blur/composite.comp.spv in assets/shaders/");
        return false;
    }

    if (!CreatePipelines()) {
        LOG_ERROR("BloomPostProcessPass", "Failed to create compute pipelines");
        return false;
    }

    SamplerDesc samplerDesc;
    samplerDesc.filter = TextureFilter::Linear;
    samplerDesc.addressU = TextureAddressMode::Clamp;
    samplerDesc.addressV = TextureAddressMode::Clamp;
    samplerDesc.addressW = TextureAddressMode::Clamp;
    m_linearSampler.reset(m_factory->CreateSamplerImpl(samplerDesc).release());

    m_ready = true;
    LOG_INFO("BloomPostProcessPass", "Bloom initialized successfully");
    return true;
}

void BloomPostProcessPass::Execute(ICommandBuffer* cmd, ITexture* source, ITextureRenderTarget* target) {
    if (!m_ready || !cmd || !source || !target) return;

    ITexture* output = target->GetTexture();
    if (!output) return;

    uint32_t w = static_cast<uint32_t>(source->GetWidth());
    uint32_t h = static_cast<uint32_t>(source->GetHeight());
    if (w == 0 || h == 0) return;

    if (w != m_width || h != m_height) {
        if (!CreateTextures(w, h)) {
            LOG_WARN("BloomPostProcessPass", "Failed to recreate textures for {}x{}", w, h);
            return;
        }
        if (!CreateDescriptorSets(w, h)) {
            LOG_WARN("BloomPostProcessPass", "Failed to recreate descriptor sets for {}x{}", w, h);
            return;
        }
        m_width = w;
        m_height = h;
    }

    // Stage 1: Prefilter
    {
        std::vector<ImageBarrier> barriers;
        barriers.push_back({source, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
        barriers.push_back({m_brightTexture.get(), ResourceState::ShaderRead, ResourceState::UnorderedAccess});
        cmd->PipelineBarrier(barriers);

        cmd->SetComputePipeline(m_prefilterPipeline.get());
        cmd->BindDescriptorSet(0, m_prefilterDescSet.get());

        struct { float threshold; } pc;
        pc.threshold = m_threshold;
        cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

        uint32_t gw = (w / 2 + 15) / 16;
        uint32_t gh = (h / 2 + 15) / 16;
        cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);
    }

    // Stage 2: Kawase Blur (ping-pong)
    {
        ITexture* readTex = m_brightTexture.get();
        ITexture* writeTex = m_tempTextureA.get();
        uint32_t hw = w / 2;
        uint32_t hh = h / 2;
        uint32_t gw = (hw + 15) / 16;
        uint32_t gh = (hh + 15) / 16;

        for (int i = 0; i < m_blurIterations; i++) {
            m_blurDescSet->BindTexture(0, readTex, m_linearSampler.get());
            m_blurDescSet->BindStorageImage(1, writeTex);
            m_blurDescSet->Update();

            std::vector<ImageBarrier> barriers;
            barriers.push_back({readTex, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
            barriers.push_back({writeTex, ResourceState::ShaderRead, ResourceState::UnorderedAccess});
            cmd->PipelineBarrier(barriers);

            cmd->SetComputePipeline(m_blurPipeline.get());
            cmd->BindDescriptorSet(0, m_blurDescSet.get());

            struct { float radius; } pc;
            pc.radius = m_blurRadius;
            cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

            cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);

            std::swap(readTex, writeTex);
        }

        // readTex is now the last-written texture, i.e. the final blurred result
        ITexture* finalBlurred = readTex;

        // Stage 3: Composite
        {
            m_compositeDescSet->BindTexture(0, source, m_linearSampler.get());
            m_compositeDescSet->BindTexture(1, finalBlurred, m_linearSampler.get());
            m_compositeDescSet->BindStorageImage(2, output);
            m_compositeDescSet->Update();

            std::vector<ImageBarrier> barriers;
            barriers.push_back({source, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
            barriers.push_back({finalBlurred, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
            barriers.push_back({output, ResourceState::ShaderRead, ResourceState::UnorderedAccess});
            cmd->PipelineBarrier(barriers);

            cmd->SetComputePipeline(m_compositePipeline.get());
            cmd->BindDescriptorSet(0, m_compositeDescSet.get());

            uint32_t gwFull = (w + 15) / 16;
            uint32_t ghFull = (h + 15) / 16;
            cmd->Dispatch(std::max(gwFull, 1u), std::max(ghFull, 1u), 1);
        }
    }
}

void BloomPostProcessPass::Cleanup() {
    m_prefilterPipeline.reset();
    m_blurPipeline.reset();
    m_compositePipeline.reset();

    m_prefilterShader.reset();
    m_blurShader.reset();
    m_compositeShader.reset();

    m_brightTexture.reset();
    m_tempTextureA.reset();
    m_tempTextureB.reset();

    m_prefilterDescSet.reset();
    m_prefilterDescSetLayout.reset();
    m_blurDescSet.reset();
    m_blurDescSetLayout.reset();
    m_compositeDescSet.reset();
    m_compositeDescSetLayout.reset();

    m_linearSampler.reset();

    m_ready = false;
    m_width = 0;
    m_height = 0;
    m_device = nullptr;
    m_factory = nullptr;
}

bool BloomPostProcessPass::CreateTextures(uint32_t width, uint32_t height) {
    if (!m_factory) return false;

    uint32_t halfW = std::max(width / 2, 1u);
    uint32_t halfH = std::max(height / 2, 1u);

    TextureDesc brightDesc;
    brightDesc.type = TextureType::Texture2D;
    brightDesc.format = TextureFormat::RGBA16_Float;
    brightDesc.width = halfW;
    brightDesc.height = halfH;
    brightDesc.depth = 1;
    brightDesc.mipLevels = 1;
    brightDesc.arraySize = 1;
    brightDesc.allowRenderTarget = false;
    brightDesc.allowUnorderedAccess = true;
    brightDesc.allowShaderResource = true;

    m_brightTexture.reset(m_factory->CreateTextureImpl(brightDesc).release());
    m_tempTextureA.reset(m_factory->CreateTextureImpl(brightDesc).release());
    m_tempTextureB.reset(m_factory->CreateTextureImpl(brightDesc).release());

    if (!m_brightTexture || !m_tempTextureA || !m_tempTextureB) {
        LOG_WARN("BloomPostProcessPass", "Failed to create temporary textures");
        return false;
    }

    return true;
}

bool BloomPostProcessPass::CreatePipelines() {
    if (!m_factory || !m_device) return false;

    m_prefilterPipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_prefilterPipeline->SetShader(m_prefilterShader);
    m_prefilterPipeline->SetPushConstantRange(sizeof(float));
    if (!m_prefilterPipeline->Create(m_device)) {
        LOG_ERROR("BloomPostProcessPass", "Failed to create prefilter compute pipeline");
        return false;
    }

    m_blurPipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_blurPipeline->SetShader(m_blurShader);
    m_blurPipeline->SetPushConstantRange(sizeof(float));
    if (!m_blurPipeline->Create(m_device)) {
        LOG_ERROR("BloomPostProcessPass", "Failed to create blur compute pipeline");
        return false;
    }

    m_compositePipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_compositePipeline->SetShader(m_compositeShader);
    m_compositePipeline->SetPushConstantRange(0);
    if (!m_compositePipeline->Create(m_device)) {
        LOG_ERROR("BloomPostProcessPass", "Failed to create composite compute pipeline");
        return false;
    }

    return true;
}

bool BloomPostProcessPass::CreateDescriptorSets(uint32_t width, uint32_t height) {
    if (!m_factory) return false;

    {
        auto layouts = m_prefilterPipeline->GetDescriptorSetLayouts();
        if (layouts.empty()) return false;

        m_prefilterDescSetLayout = layouts[0];
        m_prefilterDescSet = m_factory->CreateDescriptorSet(m_prefilterDescSetLayout.get());
        if (!m_prefilterDescSet) return false;

        m_prefilterDescSet->BindTexture(0, nullptr, nullptr);
        m_prefilterDescSet->BindStorageImage(1, m_brightTexture.get());
        m_prefilterDescSet->Update();
    }

    {
        auto layouts = m_blurPipeline->GetDescriptorSetLayouts();
        if (layouts.empty()) return false;

        m_blurDescSetLayout = layouts[0];
        m_blurDescSet = m_factory->CreateDescriptorSet(m_blurDescSetLayout.get());
        if (!m_blurDescSet) return false;

        m_blurDescSet->BindTexture(0, nullptr, nullptr);
        m_blurDescSet->BindStorageImage(1, nullptr);
        m_blurDescSet->Update();
    }

    {
        auto layouts = m_compositePipeline->GetDescriptorSetLayouts();
        if (layouts.empty()) return false;

        m_compositeDescSetLayout = layouts[0];
        m_compositeDescSet = m_factory->CreateDescriptorSet(m_compositeDescSetLayout.get());
        if (!m_compositeDescSet) return false;

        m_compositeDescSet->BindTexture(0, nullptr, nullptr);
        m_compositeDescSet->BindTexture(1, nullptr, nullptr);
        m_compositeDescSet->BindStorageImage(2, nullptr);
        m_compositeDescSet->Update();
    }

    return true;
}

} // namespace Prisma::Graphic
