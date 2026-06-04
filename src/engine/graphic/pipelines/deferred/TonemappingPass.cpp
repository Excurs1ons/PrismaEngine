#include "TonemappingPass.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "app/Engine.h"
#include "Logger.h"
#include <cstdint>
#include <vector>

namespace Prisma::Graphic {

TonemappingPass::TonemappingPass() = default;

TonemappingPass::~TonemappingPass() {
    Cleanup();
}

bool TonemappingPass::Setup(IRenderDevice* device) {
    if (!device) return false;
    m_device = device;
    m_factory = device->GetResourceFactory();
    if (!m_factory) return false;

    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rm) return false;

    m_tonemapShader = rm->LoadShaderSync("assets/shaders/tonemapping.comp.spv");

    if (!m_tonemapShader) {
        LOG_WARN("TonemappingPass", "tonemapping.comp.spv not found, tonemapping disabled. "
                  "Place shader in assets/shaders/");
        return false;
    }

    if (!CreatePipelines()) {
        LOG_ERROR("TonemappingPass", "Failed to create compute pipeline");
        return false;
    }

    {
        SamplerDesc linearDesc;
        linearDesc.filter = TextureFilter::Linear;
        linearDesc.addressU = TextureAddressMode::Clamp;
        linearDesc.addressV = TextureAddressMode::Clamp;
        linearDesc.addressW = TextureAddressMode::Clamp;
        m_linearSampler.reset(m_factory->CreateSamplerImpl(linearDesc).release());
    }

    if (!CreateDescriptorSets()) {
        LOG_ERROR("TonemappingPass", "Failed to create descriptor sets");
        return false;
    }

    m_ready = true;
    LOG_INFO("TonemappingPass", "Tonemapping initialized (exposure={}, gamma={})",
             m_exposure, m_gamma);
    return true;
}

void TonemappingPass::Execute(ICommandBuffer* cmd, ITexture* input, ITexture* output) {
    if (!m_ready || !cmd || !input || !output) return;
    if (!m_enabled) return;

    uint32_t w = static_cast<uint32_t>(output->GetWidth());
    uint32_t h = static_cast<uint32_t>(output->GetHeight());
    if (w == 0 || h == 0) return;

    m_width = w;
    m_height = h;

    // Bind input and output
    m_tonemapDescSet->BindTexture(0, input, m_linearSampler.get());
    m_tonemapDescSet->BindStorageImage(1, output);
    m_tonemapDescSet->Update();

    // Pipeline barriers
    std::vector<ImageBarrier> barriers;
    barriers.push_back({input, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
    barriers.push_back({output, ResourceState::ShaderRead, ResourceState::UnorderedAccess});
    cmd->PipelineBarrier(barriers);

    cmd->SetComputePipeline(m_tonemapPipeline.get());
    cmd->BindDescriptorSet(0, m_tonemapDescSet.get());

    // Push constants: exposure, gamma
    struct TonemapParams {
        float exposure;
        float gamma;
    } pc;
    pc.exposure = m_exposure;
    pc.gamma = m_gamma;
    cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

    uint32_t gw = (w + 15) / 16;
    uint32_t gh = (h + 15) / 16;
    cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);
}

void TonemappingPass::Cleanup() {
    m_tonemapPipeline.reset();
    m_tonemapShader.reset();

    m_tonemapDescSet.reset();
    m_tonemapDescSetLayout.reset();

    m_linearSampler.reset();

    m_ready = false;
    m_width = 0;
    m_height = 0;
    m_device = nullptr;
    m_factory = nullptr;
}

bool TonemappingPass::CreatePipelines() {
    if (!m_factory || !m_device) return false;

    m_tonemapPipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_tonemapPipeline->SetShader(m_tonemapShader);
    m_tonemapPipeline->SetPushConstantRange(sizeof(float) * 2);
    if (!m_tonemapPipeline->Create(m_device)) {
        LOG_ERROR("TonemappingPass", "Failed to create tonemapping compute pipeline");
        return false;
    }

    return true;
}

bool TonemappingPass::CreateDescriptorSets() {
    if (!m_factory) return false;

    auto layouts = m_tonemapPipeline->GetDescriptorSetLayouts();
    if (layouts.empty()) return false;

    m_tonemapDescSetLayout = layouts[0];
    m_tonemapDescSet = m_factory->CreateDescriptorSet(m_tonemapDescSetLayout.get());
    if (!m_tonemapDescSet) return false;

    m_tonemapDescSet->BindTexture(0, nullptr, nullptr);
    m_tonemapDescSet->BindStorageImage(1, nullptr);
    m_tonemapDescSet->Update();

    return true;
}

} // namespace Prisma::Graphic
