#include "SSRPass.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "app/Engine.h"
#include "Logger.h"
#include <cstdint>
#include <vector>

namespace Prisma::Graphic {

SSRPass::SSRPass() = default;

SSRPass::~SSRPass() {
    Cleanup();
}

bool SSRPass::Setup(IRenderDevice* device) {
    if (!device) return false;
    m_device = device;
    m_factory = device->GetResourceFactory();
    if (!m_factory) return false;

    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rm) return false;

    m_ssrShader = rm->LoadShaderSync("assets/shaders/ssr_main.comp.spv");

    if (!m_ssrShader) {
        LOG_WARN("SSRPass", "ssr_main.comp.spv not found, SSR disabled. "
                  "Place shader in assets/shaders/");
        return false;
    }

    if (!CreatePipelines()) {
        LOG_ERROR("SSRPass", "Failed to create compute pipeline");
        return false;
    }

    // 创建采样器
    {
        SamplerDesc pointDesc;
        pointDesc.filter = TextureFilter::Point;
        pointDesc.addressU = TextureAddressMode::Clamp;
        pointDesc.addressV = TextureAddressMode::Clamp;
        pointDesc.addressW = TextureAddressMode::Clamp;
        m_pointSampler.reset(m_factory->CreateSamplerImpl(pointDesc).release());
    }
    {
        SamplerDesc linearDesc;
        linearDesc.filter = TextureFilter::Linear;
        linearDesc.addressU = TextureAddressMode::Clamp;
        linearDesc.addressV = TextureAddressMode::Clamp;
        linearDesc.addressW = TextureAddressMode::Clamp;
        m_linearSampler.reset(m_factory->CreateSamplerImpl(linearDesc).release());
    }

    m_ready = true;
    LOG_INFO("SSRPass", "SSR initialized successfully (steps={}, thickness={}, maxDist={})",
             m_stepCount, m_thickness, m_maxDistance);
    return true;
}

void SSRPass::Execute(ICommandBuffer* cmd, ITexture* gbColor, ITexture* gbNormal,
                      ITexture* gbDepth, ITexture* gbPosition, ITexture* output) {
    if (!m_ready || !cmd || !gbColor || !gbNormal || !gbDepth || !gbPosition || !output) return;
    if (!m_enabled) return;

    uint32_t w = static_cast<uint32_t>(output->GetWidth());
    uint32_t h = static_cast<uint32_t>(output->GetHeight());
    if (w == 0 || h == 0) return;

    if (w != m_width || h != m_height) {
        if (!CreateTextures(w, h)) {
            LOG_WARN("SSRPass", "Failed to recreate textures for {}x{}", w, h);
            return;
        }
        if (!CreateDescriptorSets(w, h)) {
            LOG_WARN("SSRPass", "Failed to recreate descriptor sets for {}x{}", w, h);
            return;
        }
        m_width = w;
        m_height = h;
    }

    // Bind G-buffer textures and output
    m_ssrDescSet->BindTexture(0, gbColor, m_linearSampler.get());
    m_ssrDescSet->BindTexture(1, gbNormal, m_pointSampler.get());
    m_ssrDescSet->BindTexture(2, gbDepth, m_pointSampler.get());
    m_ssrDescSet->BindTexture(3, gbPosition, m_pointSampler.get());
    m_ssrDescSet->BindStorageImage(4, m_reflectionTexture.get());
    m_ssrDescSet->Update();

    // Pipeline barriers
    std::vector<ImageBarrier> barriers;
    barriers.push_back({gbColor, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
    barriers.push_back({gbNormal, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
    barriers.push_back({gbDepth, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
    barriers.push_back({gbPosition, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
    barriers.push_back({m_reflectionTexture.get(), ResourceState::ShaderRead, ResourceState::UnorderedAccess});
    cmd->PipelineBarrier(barriers);

    cmd->SetComputePipeline(m_ssrPipeline.get());
    cmd->BindDescriptorSet(0, m_ssrDescSet.get());

    // Push constants: stepCount, thickness, maxDistance, fadeFactor
    struct SSRParams {
        float stepCount;
        float thickness;
        float maxDistance;
        float fadeFactor;
    } pc;
    pc.stepCount = m_stepCount;
    pc.thickness = m_thickness;
    pc.maxDistance = m_maxDistance;
    pc.fadeFactor = m_fadeFactor;
    cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

    uint32_t gw = (w + 15) / 16;
    uint32_t gh = (h + 15) / 16;
    cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);
}

void SSRPass::Cleanup() {
    m_ssrPipeline.reset();
    m_ssrShader.reset();

    m_reflectionTexture.reset();

    m_ssrDescSet.reset();
    m_ssrDescSetLayout.reset();

    m_pointSampler.reset();
    m_linearSampler.reset();

    m_ready = false;
    m_width = 0;
    m_height = 0;
    m_device = nullptr;
    m_factory = nullptr;
}

bool SSRPass::CreateTextures(uint32_t width, uint32_t height) {
    if (!m_factory) return false;

    TextureDesc reflectionDesc;
    reflectionDesc.type = TextureType::Texture2D;
    reflectionDesc.format = TextureFormat::RGBA16_Float;
    reflectionDesc.width = width;
    reflectionDesc.height = height;
    reflectionDesc.depth = 1;
    reflectionDesc.mipLevels = 1;
    reflectionDesc.arraySize = 1;
    reflectionDesc.allowRenderTarget = false;
    reflectionDesc.allowUnorderedAccess = true;
    reflectionDesc.allowShaderResource = true;

    m_reflectionTexture.reset(m_factory->CreateTextureImpl(reflectionDesc).release());
    if (!m_reflectionTexture) {
        LOG_WARN("SSRPass", "Failed to create reflection texture for {}x{}", width, height);
        return false;
    }

    return true;
}

bool SSRPass::CreatePipelines() {
    if (!m_factory || !m_device) return false;

    m_ssrPipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_ssrPipeline->SetShader(m_ssrShader);
    m_ssrPipeline->SetPushConstantRange(sizeof(float) * 4);
    if (!m_ssrPipeline->Create(m_device)) {
        LOG_ERROR("SSRPass", "Failed to create SSR compute pipeline");
        return false;
    }

    return true;
}

bool SSRPass::CreateDescriptorSets(uint32_t width, uint32_t height) {
    if (!m_factory) return false;

    auto layouts = m_ssrPipeline->GetDescriptorSetLayouts();
    if (layouts.empty()) return false;

    m_ssrDescSetLayout = layouts[0];
    m_ssrDescSet = m_factory->CreateDescriptorSet(m_ssrDescSetLayout.get());
    if (!m_ssrDescSet) return false;

    // Bindings: gbColor(0), gbNormal(1), gbDepth(2), gbPosition(3), output(4)
    m_ssrDescSet->BindTexture(0, nullptr, nullptr);
    m_ssrDescSet->BindTexture(1, nullptr, nullptr);
    m_ssrDescSet->BindTexture(2, nullptr, nullptr);
    m_ssrDescSet->BindTexture(3, nullptr, nullptr);
    m_ssrDescSet->BindStorageImage(4, m_reflectionTexture.get());
    m_ssrDescSet->Update();

    return true;
}

} // namespace Prisma::Graphic
