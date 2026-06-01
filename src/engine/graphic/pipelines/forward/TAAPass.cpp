#include "TAAPass.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "app/Engine.h"
#include "Logger.h"
#include <cstdint>
#include <vector>

namespace Prisma::Graphic {

TAAPass::TAAPass() = default;

TAAPass::~TAAPass() {
    Cleanup();
}

bool TAAPass::Setup(IRenderDevice* device) {
    if (!device) return false;
    m_device = device;
    m_factory = device->GetResourceFactory();
    if (!m_factory) return false;

    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rm) return false;

    m_taaShader = rm->LoadShaderSync("assets/shaders/taa_resolve.comp.spv");

    if (!m_taaShader) {
        LOG_WARN("TAAPass", "taa_resolve.comp.spv not found, TAA disabled. "
                  "Place shader in assets/shaders/");
        return false;
    }

    if (!CreatePipelines()) {
        LOG_ERROR("TAAPass", "Failed to create compute pipeline");
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
    m_historyValid = false;
    LOG_INFO("TAAPass", "TAA initialized successfully (blend={}, sharpness={})",
             m_blendFactor, m_sharpness);
    return true;
}

void TAAPass::Execute(ICommandBuffer* cmd, ITexture* currentFrame, ITexture* motionVectors,
                      ITexture* output) {
    if (!m_ready || !cmd || !currentFrame || !output) return;

    uint32_t w = static_cast<uint32_t>(output->GetWidth());
    uint32_t h = static_cast<uint32_t>(output->GetHeight());
    if (w == 0 || h == 0) return;

    if (w != m_width || h != m_height) {
        if (!CreateTextures(w, h)) {
            LOG_WARN("TAAPass", "Failed to recreate textures for {}x{}", w, h);
            return;
        }
        if (!CreateDescriptorSets(w, h)) {
            LOG_WARN("TAAPass", "Failed to recreate descriptor sets for {}x{}", w, h);
            return;
        }
        m_width = w;
        m_height = h;
    }

    // 如果没有有效历史，使用当前帧作为历史并直接输出
    if (!m_historyValid) {
        // 直接拷贝当前帧到输出和历史缓冲
        std::vector<ImageBarrier> copyBarriersOut;
        copyBarriersOut.push_back({currentFrame, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
        copyBarriersOut.push_back({output, ResourceState::ShaderRead, ResourceState::UnorderedAccess});
        cmd->PipelineBarrier(copyBarriersOut);

        // 对第一帧使用简单的拷贝 dispatch
        // 使用 TAA 管线但传递特殊标记: blendFactor=0 表示不混合
        {
            m_taaDescSet->BindTexture(0, currentFrame, m_linearSampler.get());
            m_taaDescSet->BindTexture(1, currentFrame, m_linearSampler.get()); // 无历史，用当前帧
            m_taaDescSet->BindTexture(2, motionVectors ? motionVectors : currentFrame, m_pointSampler.get());
            m_taaDescSet->BindStorageImage(3, output);
            m_taaDescSet->Update();

            std::vector<ImageBarrier> barriers;
            barriers.push_back({currentFrame, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
            barriers.push_back({output, ResourceState::ShaderRead, ResourceState::UnorderedAccess});
            if (motionVectors) {
                barriers.push_back({motionVectors, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
            }
            cmd->PipelineBarrier(barriers);

            cmd->SetComputePipeline(m_taaPipeline.get());
            cmd->BindDescriptorSet(0, m_taaDescSet.get());

            struct TAAParams {
                float blendFactor;
                float jitterScale;
                float sharpness;
                int firstFrame;
            } pc;
            pc.blendFactor = 0.0f;   // 第一帧不混合
            pc.jitterScale = m_jitterScale;
            pc.sharpness = m_sharpness;
            pc.firstFrame = 1;
            cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

            uint32_t gw = (w + 15) / 16;
            uint32_t gh = (h + 15) / 16;
            cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);
        }

        // 更新历史纹理
        {
            std::vector<ImageBarrier> barriers;
            barriers.push_back({output, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
            barriers.push_back({m_historyTextureA.get(), ResourceState::ShaderRead, ResourceState::UnorderedAccess});
            cmd->PipelineBarrier(barriers);

            cmd->SetComputePipeline(m_taaPipeline.get());

            m_taaDescSet->BindTexture(0, output, m_linearSampler.get());
            m_taaDescSet->BindTexture(1, output, m_linearSampler.get());
            m_taaDescSet->BindTexture(2, output, m_pointSampler.get());
            m_taaDescSet->BindStorageImage(3, m_historyTextureA.get());
            m_taaDescSet->Update();

            struct TAAParams {
                float blendFactor;
                float jitterScale;
                float sharpness;
                int firstFrame;
            } pc;
            pc.blendFactor = 0.0f;
            pc.jitterScale = m_jitterScale;
            pc.sharpness = m_sharpness;
            pc.firstFrame = 1;
            cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

            uint32_t gw = (w + 15) / 16;
            uint32_t gh = (h + 15) / 16;
            cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);
        }

        m_historyValid = true;
        return;
    }

    // 正常 TAA 处理: 历史帧存在于 m_historyTextureA 中
    ITexture* history = m_historyTextureA.get();

    // TAA Resolve: 混合当前帧和历史帧
    {
        m_taaDescSet->BindTexture(0, currentFrame, m_linearSampler.get());
        m_taaDescSet->BindTexture(1, history, m_linearSampler.get());
        m_taaDescSet->BindTexture(2, motionVectors ? motionVectors : currentFrame, m_pointSampler.get());
        m_taaDescSet->BindStorageImage(3, output);
        m_taaDescSet->Update();

        std::vector<ImageBarrier> barriers;
        barriers.push_back({currentFrame, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
        barriers.push_back({history, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
        barriers.push_back({output, ResourceState::ShaderRead, ResourceState::UnorderedAccess});
        if (motionVectors) {
            barriers.push_back({motionVectors, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
        }
        cmd->PipelineBarrier(barriers);

        cmd->SetComputePipeline(m_taaPipeline.get());
        cmd->BindDescriptorSet(0, m_taaDescSet.get());

        struct TAAParams {
            float blendFactor;
            float jitterScale;
            float sharpness;
            int firstFrame;
        } pc;
        pc.blendFactor = m_blendFactor;
        pc.jitterScale = m_jitterScale;
        pc.sharpness = m_sharpness;
        pc.firstFrame = 0;
        cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

        uint32_t gw = (w + 15) / 16;
        uint32_t gh = (h + 15) / 16;
        cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);
    }

    // 将输出拷贝到历史纹理供下帧使用
    {
        std::vector<ImageBarrier> barriers;
        barriers.push_back({output, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
        barriers.push_back({m_historyTextureB.get(), ResourceState::ShaderRead, ResourceState::UnorderedAccess});
        cmd->PipelineBarrier(barriers);

        m_taaDescSet->BindTexture(0, output, m_linearSampler.get());
        m_taaDescSet->BindTexture(1, output, m_linearSampler.get());
        m_taaDescSet->BindTexture(2, output, m_pointSampler.get());
        m_taaDescSet->BindStorageImage(3, m_historyTextureB.get());
        m_taaDescSet->Update();

        struct TAAParams {
            float blendFactor;
            float jitterScale;
            float sharpness;
            int firstFrame;
        } pc;
        pc.blendFactor = 0.0f;
        pc.jitterScale = m_jitterScale;
        pc.sharpness = m_sharpness;
        pc.firstFrame = 1;
        cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

        uint32_t gw = (w + 15) / 16;
        uint32_t gh = (h + 15) / 16;
        cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);
    }

    // 交换 A/B 历史纹理
    std::swap(m_historyTextureA, m_historyTextureB);
}

void TAAPass::Cleanup() {
    m_taaPipeline.reset();
    m_taaShader.reset();

    m_historyTextureA.reset();
    m_historyTextureB.reset();

    m_taaDescSet.reset();
    m_taaDescSetLayout.reset();

    m_pointSampler.reset();
    m_linearSampler.reset();

    m_ready = false;
    m_historyValid = false;
    m_width = 0;
    m_height = 0;
    m_device = nullptr;
    m_factory = nullptr;
}

bool TAAPass::CreateTextures(uint32_t width, uint32_t height) {
    if (!m_factory) return false;

    TextureDesc historyDesc;
    historyDesc.type = TextureType::Texture2D;
    historyDesc.format = TextureFormat::RGBA16_Float;
    historyDesc.width = width;
    historyDesc.height = height;
    historyDesc.depth = 1;
    historyDesc.mipLevels = 1;
    historyDesc.arraySize = 1;
    historyDesc.allowRenderTarget = false;
    historyDesc.allowUnorderedAccess = true;
    historyDesc.allowShaderResource = true;

    m_historyTextureA.reset(m_factory->CreateTextureImpl(historyDesc).release());
    m_historyTextureB.reset(m_factory->CreateTextureImpl(historyDesc).release());

    if (!m_historyTextureA || !m_historyTextureB) {
        LOG_WARN("TAAPass", "Failed to create history textures for {}x{}", width, height);
        return false;
    }

    return true;
}

bool TAAPass::CreatePipelines() {
    if (!m_factory || !m_device) return false;

    m_taaPipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_taaPipeline->SetShader(m_taaShader);
    m_taaPipeline->SetPushConstantRange(sizeof(float) * 4);
    if (!m_taaPipeline->Create(m_device)) {
        LOG_ERROR("TAAPass", "Failed to create TAA compute pipeline");
        return false;
    }

    return true;
}

bool TAAPass::CreateDescriptorSets(uint32_t width, uint32_t height) {
    if (!m_factory) return false;

    auto layouts = m_taaPipeline->GetDescriptorSetLayouts();
    if (layouts.empty()) return false;

    m_taaDescSetLayout = layouts[0];
    m_taaDescSet = m_factory->CreateDescriptorSet(m_taaDescSetLayout.get());
    if (!m_taaDescSet) return false;

    m_taaDescSet->BindTexture(0, nullptr, nullptr);
    m_taaDescSet->BindTexture(1, nullptr, nullptr);
    m_taaDescSet->BindTexture(2, nullptr, nullptr);
    m_taaDescSet->BindStorageImage(3, nullptr);
    m_taaDescSet->Update();

    return true;
}

} // namespace Prisma::Graphic
