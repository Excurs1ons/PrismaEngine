#include "OutlinePostProcessPass.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "app/Engine.h"
#include "Logger.h"

namespace Prisma::Graphic {

OutlinePostProcessPass::OutlinePostProcessPass() = default;

OutlinePostProcessPass::~OutlinePostProcessPass() {
    Cleanup();
}

bool OutlinePostProcessPass::Setup(IRenderDevice* device) {
    if (!device) return false;
    m_device = device;
    m_factory = device->GetResourceFactory();
    if (!m_factory) return false;

    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rm) return false;

    m_edgeShader = rm->LoadShaderSync("assets/shaders/npr_outline_edge.comp.spv");
    m_compositeShader = rm->LoadShaderSync("assets/shaders/npr_outline_composite.comp.spv");

    if (!m_edgeShader || !m_compositeShader) {
        LOG_WARN("OutlinePostProcessPass", "Outline shaders not found, outline disabled. "
                  "Place npr_outline_edge/composite.comp.spv in assets/shaders/");
        return false;
    }

    if (!CreatePipelines()) {
        LOG_ERROR("OutlinePostProcessPass", "Failed to create compute pipelines");
        return false;
    }

    SamplerDesc samplerDesc;
    samplerDesc.filter = TextureFilter::Linear;
    samplerDesc.addressU = TextureAddressMode::Clamp;
    samplerDesc.addressV = TextureAddressMode::Clamp;
    samplerDesc.addressW = TextureAddressMode::Clamp;
    m_linearSampler.reset(m_factory->CreateSamplerImpl(samplerDesc).release());

    m_ready = true;
    LOG_INFO("OutlinePostProcessPass", "Outline initialized successfully");
    return true;
}

void OutlinePostProcessPass::Execute(ICommandBuffer* cmd, ITexture* colorInput, ITextureRenderTarget* output) {
    if (!m_ready || !cmd || !colorInput || !output) return;

    ITexture* outputTex = output->GetTexture();
    if (!outputTex) return;

    uint32_t w = static_cast<uint32_t>(colorInput->GetWidth());
    uint32_t h = static_cast<uint32_t>(colorInput->GetHeight());
    if (w == 0 || h == 0) return;

    if (w != m_width || h != m_height) {
        if (!CreateTextures(w, h)) {
            LOG_WARN("OutlinePostProcessPass", "Failed to recreate textures for {}x{}", w, h);
            return;
        }
        if (!CreateDescriptorSets(w, h)) {
            LOG_WARN("OutlinePostProcessPass", "Failed to recreate descriptor sets for {}x{}", w, h);
            return;
        }
        m_width = w;
        m_height = h;
    }

    // Stage 1: Edge detection
    {
        std::vector<ImageBarrier> barriers;
        barriers.push_back({colorInput, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
        barriers.push_back({m_edgeTexture.get(), ResourceState::ShaderRead, ResourceState::UnorderedAccess});
        cmd->PipelineBarrier(barriers);

        cmd->SetComputePipeline(m_edgePipeline.get());
        cmd->BindDescriptorSet(0, m_edgeDescSet.get());

        EdgePushConstants pc;
        pc.color[0] = m_outlineColor.r;
        pc.color[1] = m_outlineColor.g;
        pc.color[2] = m_outlineColor.b;
        pc.color[3] = m_outlineColor.a;
        pc.width = m_outlineWidth;
        cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

        uint32_t gw = (w + 15) / 16;
        uint32_t gh = (h + 15) / 16;
        cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);
    }

    // Stage 2: Composite outline onto original
    {
        m_compositeDescSet->BindTexture(0, colorInput, m_linearSampler.get());
        m_compositeDescSet->BindTexture(1, m_edgeTexture.get(), m_linearSampler.get());
        m_compositeDescSet->BindStorageImage(2, outputTex);
        m_compositeDescSet->Update();

        std::vector<ImageBarrier> barriers;
        barriers.push_back({colorInput, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
        barriers.push_back({m_edgeTexture.get(), ResourceState::UnorderedAccess, ResourceState::ShaderRead});
        barriers.push_back({outputTex, ResourceState::ShaderRead, ResourceState::UnorderedAccess});
        cmd->PipelineBarrier(barriers);

        cmd->SetComputePipeline(m_compositePipeline.get());
        cmd->BindDescriptorSet(0, m_compositeDescSet.get());

        uint32_t gw = (w + 15) / 16;
        uint32_t gh = (h + 15) / 16;
        cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);
    }
}

void OutlinePostProcessPass::Cleanup() {
    m_edgePipeline.reset();
    m_compositePipeline.reset();

    m_edgeShader.reset();
    m_compositeShader.reset();

    m_edgeTexture.reset();

    m_edgeDescSet.reset();
    m_edgeDescSetLayout.reset();
    m_compositeDescSet.reset();
    m_compositeDescSetLayout.reset();

    m_linearSampler.reset();

    m_ready = false;
    m_width = 0;
    m_height = 0;
    m_device = nullptr;
    m_factory = nullptr;
}

bool OutlinePostProcessPass::CreateTextures(uint32_t width, uint32_t height) {
    if (!m_factory) return false;

    uint32_t halfW = std::max(width / 2, 1u);
    uint32_t halfH = std::max(height / 2, 1u);

    TextureDesc edgeDesc;
    edgeDesc.type = TextureType::Texture2D;
    edgeDesc.format = TextureFormat::RGBA16_Float;
    edgeDesc.width = halfW;
    edgeDesc.height = halfH;
    edgeDesc.depth = 1;
    edgeDesc.mipLevels = 1;
    edgeDesc.arraySize = 1;
    edgeDesc.allowRenderTarget = false;
    edgeDesc.allowUnorderedAccess = true;
    edgeDesc.allowShaderResource = true;

    m_edgeTexture.reset(m_factory->CreateTextureImpl(edgeDesc).release());

    if (!m_edgeTexture) {
        LOG_WARN("OutlinePostProcessPass", "Failed to create edge texture");
        return false;
    }

    return true;
}

bool OutlinePostProcessPass::CreatePipelines() {
    if (!m_factory || !m_device) return false;

    m_edgePipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_edgePipeline->SetShader(m_edgeShader);
    m_edgePipeline->SetPushConstantRange(sizeof(EdgePushConstants));
    if (!m_edgePipeline->Create(m_device)) {
        LOG_ERROR("OutlinePostProcessPass", "Failed to create edge detection compute pipeline");
        return false;
    }

    m_compositePipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_compositePipeline->SetShader(m_compositeShader);
    m_compositePipeline->SetPushConstantRange(0);
    if (!m_compositePipeline->Create(m_device)) {
        LOG_ERROR("OutlinePostProcessPass", "Failed to create composite compute pipeline");
        return false;
    }

    return true;
}

bool OutlinePostProcessPass::CreateDescriptorSets(uint32_t width, uint32_t height) {
    if (!m_factory) return false;
    (void)width;
    (void)height;

    {
        auto layouts = m_edgePipeline->GetDescriptorSetLayouts();
        if (layouts.empty()) return false;

        m_edgeDescSetLayout = layouts[0];
        m_edgeDescSet = m_factory->CreateDescriptorSet(m_edgeDescSetLayout.get());
        if (!m_edgeDescSet) return false;

        m_edgeDescSet->BindTexture(0, nullptr, nullptr);
        m_edgeDescSet->BindStorageImage(1, m_edgeTexture.get());
        m_edgeDescSet->Update();
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
