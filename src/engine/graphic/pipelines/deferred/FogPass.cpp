#include "FogPass.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "app/Engine.h"
#include "Logger.h"
#include <cstdint>
#include <vector>

namespace Prisma::Graphic {

FogPass::FogPass() = default;

FogPass::~FogPass() {
    Cleanup();
}

bool FogPass::Setup(IRenderDevice* device) {
    if (!device) return false;
    m_device = device;
    m_factory = device->GetResourceFactory();
    if (!m_factory) return false;

    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rm) return false;

    m_fogShader = rm->LoadShaderSync("assets/shaders/fog.comp.spv");

    if (!m_fogShader) {
        LOG_WARN("FogPass", "fog.comp.spv not found, fog disabled. "
                  "Place shader in assets/shaders/");
        return false;
    }

    if (!CreatePipelines()) {
        LOG_ERROR("FogPass", "Failed to create compute pipeline");
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
        LOG_ERROR("FogPass", "Failed to create descriptor sets");
        return false;
    }

    m_ready = true;
    LOG_INFO("FogPass", "Fog initialized (density={}, height={}, color={} {} {})",
             m_density, m_fogHeight, m_fogColor[0], m_fogColor[1], m_fogColor[2]);
    return true;
}

void FogPass::Execute(ICommandBuffer* cmd, ITexture* sceneColor, ITexture* gbDepth,
                      ITexture* gbPosition, ITexture* output) {
    if (!m_ready || !cmd || !sceneColor || !gbDepth || !gbPosition || !output) return;
    if (!m_enabled) return;

    uint32_t w = static_cast<uint32_t>(output->GetWidth());
    uint32_t h = static_cast<uint32_t>(output->GetHeight());
    if (w == 0 || h == 0) return;

    m_width = w;
    m_height = h;

    // Bind textures
    m_fogDescSet->BindTexture(0, sceneColor, m_linearSampler.get());
    m_fogDescSet->BindTexture(1, gbDepth, m_linearSampler.get());
    m_fogDescSet->BindTexture(2, gbPosition, m_linearSampler.get());
    m_fogDescSet->BindStorageImage(3, output);
    m_fogDescSet->Update();

    // Pipeline barriers
    std::vector<ImageBarrier> barriers;
    barriers.push_back({sceneColor, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
    barriers.push_back({gbDepth, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
    barriers.push_back({gbPosition, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
    barriers.push_back({output, ResourceState::ShaderRead, ResourceState::UnorderedAccess});
    cmd->PipelineBarrier(barriers);

    cmd->SetComputePipeline(m_fogPipeline.get());
    cmd->BindDescriptorSet(0, m_fogDescSet.get());

    // Push constants: density, height, fogColor[3]
    // GLSL layout has: float density, float height, float fogColorR, fogColorG, fogColorB
    struct FogParams {
        float density;
        float height;
        float fogColor[3];
    } pc;
    pc.density = m_density;
    pc.height = m_fogHeight;
    pc.fogColor[0] = m_fogColor[0];
    pc.fogColor[1] = m_fogColor[1];
    pc.fogColor[2] = m_fogColor[2];
    cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

    uint32_t gw = (w + 15) / 16;
    uint32_t gh = (h + 15) / 16;
    cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);
}

void FogPass::Cleanup() {
    m_fogPipeline.reset();
    m_fogShader.reset();

    m_fogDescSet.reset();
    m_fogDescSetLayout.reset();

    m_linearSampler.reset();

    m_ready = false;
    m_width = 0;
    m_height = 0;
    m_device = nullptr;
    m_factory = nullptr;
}

bool FogPass::CreatePipelines() {
    if (!m_factory || !m_device) return false;

    m_fogPipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_fogPipeline->SetShader(m_fogShader);
    // Push constants: density (float), height (float), fogColor[3] (float*3) = 5 floats
    m_fogPipeline->SetPushConstantRange(sizeof(float) * 5);
    if (!m_fogPipeline->Create(m_device)) {
        LOG_ERROR("FogPass", "Failed to create fog compute pipeline");
        return false;
    }

    return true;
}

bool FogPass::CreateDescriptorSets() {
    if (!m_factory) return false;

    auto layouts = m_fogPipeline->GetDescriptorSetLayouts();
    if (layouts.empty()) return false;

    m_fogDescSetLayout = layouts[0];
    m_fogDescSet = m_factory->CreateDescriptorSet(m_fogDescSetLayout.get());
    if (!m_fogDescSet) return false;

    // Bindings: sceneColor(0), gbDepth(1), gbPosition(2), output(3)
    m_fogDescSet->BindTexture(0, nullptr, nullptr);
    m_fogDescSet->BindTexture(1, nullptr, nullptr);
    m_fogDescSet->BindTexture(2, nullptr, nullptr);
    m_fogDescSet->BindStorageImage(3, nullptr);
    m_fogDescSet->Update();

    return true;
}

} // namespace Prisma::Graphic
