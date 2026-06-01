#include "SSAOPass.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "app/Engine.h"
#include "Logger.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace Prisma::Graphic {

SSAOPass::SSAOPass() = default;

SSAOPass::~SSAOPass() {
    Cleanup();
}

bool SSAOPass::Setup(IRenderDevice* device) {
    if (!device) return false;
    m_device = device;
    m_factory = device->GetResourceFactory();
    if (!m_factory) return false;

    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rm) return false;

    m_ssaoShader = rm->LoadShaderSync("assets/shaders/ssao_main.comp.spv");
    m_blurHShader = rm->LoadShaderSync("assets/shaders/ssao_blur_h.comp.spv");
    m_blurVShader = rm->LoadShaderSync("assets/shaders/ssao_blur_v.comp.spv");

    if (!m_ssaoShader) {
        LOG_WARN("SSAOPass", "ssao_main.comp.spv not found, SSAO disabled. "
                  "Place shader in assets/shaders/");
        return false;
    }

    if (!CreatePipelines()) {
        LOG_ERROR("SSAOPass", "Failed to create compute pipelines");
        return false;
    }

    // 生成半球采样核
    GenerateHemisphereSamples();

    // 创建噪声纹理
    CreateNoiseTexture();

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
    LOG_INFO("SSAOPass", "SSAO initialized successfully (samples={}, radius={}, power={})",
             m_numSamples, m_radius, m_power);
    return true;
}

void SSAOPass::Execute(ICommandBuffer* cmd, ITexture* gbPosition, ITexture* gbNormal,
                       ITexture* gbDepth, ITexture* output) {
    if (!m_ready || !cmd || !gbPosition || !gbNormal || !output) return;

    uint32_t w = static_cast<uint32_t>(output->GetWidth());
    uint32_t h = static_cast<uint32_t>(output->GetHeight());
    if (w == 0 || h == 0) return;

    if (w != m_width || h != m_height) {
        if (!CreateTextures(w, h)) {
            LOG_WARN("SSAOPass", "Failed to recreate textures for {}x{}", w, h);
            return;
        }
        if (!CreateDescriptorSets(w, h)) {
            LOG_WARN("SSAOPass", "Failed to recreate descriptor sets for {}x{}", w, h);
            return;
        }
        m_width = w;
        m_height = h;
    }

    // Stage 1: SSAO Main Pass
    {
        m_ssaoDescSet->BindTexture(0, gbPosition, m_pointSampler.get());
        m_ssaoDescSet->BindTexture(1, gbNormal, m_pointSampler.get());
        m_ssaoDescSet->BindTexture(2, gbDepth, m_pointSampler.get());
        m_ssaoDescSet->BindTexture(3, m_noiseTexture.get(), m_pointSampler.get());
        m_ssaoDescSet->BindStorageImage(4, m_aoTexture.get());
        m_ssaoDescSet->Update();

        std::vector<ImageBarrier> barriers;
        barriers.push_back({gbPosition, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
        barriers.push_back({gbNormal, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
        barriers.push_back({gbDepth, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
        barriers.push_back({m_aoTexture.get(), ResourceState::ShaderRead, ResourceState::UnorderedAccess});
        cmd->PipelineBarrier(barriers);

        cmd->SetComputePipeline(m_ssaoPipeline.get());
        cmd->BindDescriptorSet(0, m_ssaoDescSet.get());

        struct SSAOParams {
            float radius;
            float power;
            int numSamples;
            float pad;
        } pc;
        pc.radius = m_radius;
        pc.power = m_power;
        pc.numSamples = m_numSamples;
        pc.pad = 0.0f;
        cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

        uint32_t gw = (w + 15) / 16;
        uint32_t gh = (h + 15) / 16;
        cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);
    }

    // Stage 2: Separable Blur (optional)
    if (m_blurEnabled) {
        // Horizontal blur
        {
            m_blurHDescSet->BindTexture(0, m_aoTexture.get(), m_linearSampler.get());
            m_blurHDescSet->BindStorageImage(1, m_blurTempTexture.get());
            m_blurHDescSet->Update();

            std::vector<ImageBarrier> barriers;
            barriers.push_back({m_aoTexture.get(), ResourceState::UnorderedAccess, ResourceState::ShaderRead});
            barriers.push_back({m_blurTempTexture.get(), ResourceState::ShaderRead, ResourceState::UnorderedAccess});
            cmd->PipelineBarrier(barriers);

            cmd->SetComputePipeline(m_blurHPipeline.get());
            cmd->BindDescriptorSet(0, m_blurHDescSet.get());

            struct { uint32_t width; } pc;
            pc.width = w;
            cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

            uint32_t gw = (w + 15) / 16;
            uint32_t gh = (h + 15) / 16;
            cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);
        }

        // Vertical blur
        {
            m_blurVDescSet->BindTexture(0, m_blurTempTexture.get(), m_linearSampler.get());
            m_blurVDescSet->BindStorageImage(1, output);
            m_blurVDescSet->Update();

            std::vector<ImageBarrier> barriers;
            barriers.push_back({m_blurTempTexture.get(), ResourceState::UnorderedAccess, ResourceState::ShaderRead});
            barriers.push_back({output, ResourceState::ShaderRead, ResourceState::UnorderedAccess});
            cmd->PipelineBarrier(barriers);

            cmd->SetComputePipeline(m_blurVPipeline.get());
            cmd->BindDescriptorSet(0, m_blurVDescSet.get());

            struct { uint32_t height; } pc;
            pc.height = h;
            cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

            uint32_t gw = (w + 15) / 16;
            uint32_t gh = (h + 15) / 16;
            cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);
        }
    } else {
        // No blur: copy AO directly to output
        std::vector<ImageBarrier> barriers;
        barriers.push_back({m_aoTexture.get(), ResourceState::UnorderedAccess, ResourceState::ShaderRead});
        barriers.push_back({output, ResourceState::ShaderRead, ResourceState::UnorderedAccess});
        cmd->PipelineBarrier(barriers);

        // Use a simple copy or dispatch a passthrough shader.
        // For now, we bind the ao texture as input and output directly.
        // Reuse the vertical blur pipeline as passthrough (single-pass copy).
        if (m_blurVShader) {
            m_blurVDescSet->BindTexture(0, m_aoTexture.get(), m_linearSampler.get());
            m_blurVDescSet->BindStorageImage(1, output);
            m_blurVDescSet->Update();

            cmd->SetComputePipeline(m_blurVPipeline.get());
            cmd->BindDescriptorSet(0, m_blurVDescSet.get());

            struct { uint32_t height; } pc;
            pc.height = h;
            cmd->PushConstants(ShaderType::Compute, &pc, sizeof(pc));

            uint32_t gw = (w + 15) / 16;
            uint32_t gh = (h + 15) / 16;
            cmd->Dispatch(std::max(gw, 1u), std::max(gh, 1u), 1);
        }
    }
}

void SSAOPass::Cleanup() {
    m_ssaoPipeline.reset();
    m_blurHPipeline.reset();
    m_blurVPipeline.reset();

    m_ssaoShader.reset();
    m_blurHShader.reset();
    m_blurVShader.reset();

    m_aoTexture.reset();
    m_blurTempTexture.reset();
    m_noiseTexture.reset();

    m_ssaoDescSet.reset();
    m_ssaoDescSetLayout.reset();
    m_blurHDescSet.reset();
    m_blurHDescSetLayout.reset();
    m_blurVDescSet.reset();
    m_blurVDescSetLayout.reset();

    m_pointSampler.reset();
    m_linearSampler.reset();

    m_ready = false;
    m_width = 0;
    m_height = 0;
    m_device = nullptr;
    m_factory = nullptr;
}

bool SSAOPass::CreateTextures(uint32_t width, uint32_t height) {
    if (!m_factory) return false;

    // AO 纹理: 单通道 R16_Float
    TextureDesc aoDesc;
    aoDesc.type = TextureType::Texture2D;
    aoDesc.format = TextureFormat::R16_Float;
    aoDesc.width = width;
    aoDesc.height = height;
    aoDesc.depth = 1;
    aoDesc.mipLevels = 1;
    aoDesc.arraySize = 1;
    aoDesc.allowRenderTarget = false;
    aoDesc.allowUnorderedAccess = true;
    aoDesc.allowShaderResource = true;

    m_aoTexture.reset(m_factory->CreateTextureImpl(aoDesc).release());
    m_blurTempTexture.reset(m_factory->CreateTextureImpl(aoDesc).release());

    if (!m_aoTexture || !m_blurTempTexture) {
        LOG_WARN("SSAOPass", "Failed to create AO textures for {}x{}", width, height);
        return false;
    }

    return true;
}

bool SSAOPass::CreatePipelines() {
    if (!m_factory || !m_device) return false;

    // SSAO 主管线: 需要传递 4 个 PushConstant float
    m_ssaoPipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_ssaoPipeline->SetShader(m_ssaoShader);
    m_ssaoPipeline->SetPushConstantRange(sizeof(float) * 4);
    if (!m_ssaoPipeline->Create(m_device)) {
        LOG_ERROR("SSAOPass", "Failed to create ssao main compute pipeline");
        return false;
    }

    // 水平模糊管线
    m_blurHPipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_blurHPipeline->SetShader(m_blurHShader);
    m_blurHPipeline->SetPushConstantRange(sizeof(uint32_t));
    if (!m_blurHPipeline->Create(m_device)) {
        LOG_ERROR("SSAOPass", "Failed to create blur H compute pipeline");
        return false;
    }

    // 垂直模糊管线
    m_blurVPipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_blurVPipeline->SetShader(m_blurVShader);
    m_blurVPipeline->SetPushConstantRange(sizeof(uint32_t));
    if (!m_blurVPipeline->Create(m_device)) {
        LOG_ERROR("SSAOPass", "Failed to create blur V compute pipeline");
        return false;
    }

    return true;
}

bool SSAOPass::CreateDescriptorSets(uint32_t width, uint32_t height) {
    if (!m_factory) return false;

    // SSAO 主描述符集: gbPos(0), gbNormal(1), gbDepth(2), noise(3), output(4)
    {
        auto layouts = m_ssaoPipeline->GetDescriptorSetLayouts();
        if (layouts.empty()) return false;

        m_ssaoDescSetLayout = layouts[0];
        m_ssaoDescSet = m_factory->CreateDescriptorSet(m_ssaoDescSetLayout.get());
        if (!m_ssaoDescSet) return false;

        m_ssaoDescSet->BindTexture(0, nullptr, nullptr);
        m_ssaoDescSet->BindTexture(1, nullptr, nullptr);
        m_ssaoDescSet->BindTexture(2, nullptr, nullptr);
        m_ssaoDescSet->BindTexture(3, nullptr, nullptr);
        m_ssaoDescSet->BindStorageImage(4, m_aoTexture.get());
        m_ssaoDescSet->Update();
    }

    // 水平模糊描述符集: input(0), output(1)
    {
        auto layouts = m_blurHPipeline->GetDescriptorSetLayouts();
        if (layouts.empty()) return false;

        m_blurHDescSetLayout = layouts[0];
        m_blurHDescSet = m_factory->CreateDescriptorSet(m_blurHDescSetLayout.get());
        if (!m_blurHDescSet) return false;

        m_blurHDescSet->BindTexture(0, nullptr, nullptr);
        m_blurHDescSet->BindStorageImage(1, nullptr);
        m_blurHDescSet->Update();
    }

    // 垂直模糊描述符集: input(0), output(1)
    {
        auto layouts = m_blurVPipeline->GetDescriptorSetLayouts();
        if (layouts.empty()) return false;

        m_blurVDescSetLayout = layouts[0];
        m_blurVDescSet = m_factory->CreateDescriptorSet(m_blurVDescSetLayout.get());
        if (!m_blurVDescSet) return false;

        m_blurVDescSet->BindTexture(0, nullptr, nullptr);
        m_blurVDescSet->BindStorageImage(1, nullptr);
        m_blurVDescSet->Update();
    }

    return true;
}

void SSAOPass::GenerateHemisphereSamples() {
    std::mt19937 rng(42); // 固定种子保证可复现
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (int i = 0; i < 64; i++) {
        // 在半球内生成随机采样点
        float theta = 2.0f * 3.14159265f * dist(rng);
        float phi = acos(dist(rng));
        float r = pow(dist(rng), 1.0f / 3.0f); // 偏向中心以改善分布

        float x = r * sin(phi) * cos(theta);
        float y = r * sin(phi) * sin(theta);
        float z = r * cos(phi);

        // 存储为 vec4 (x, y, z, 0)
        int idx = i * 4;
        m_hemisphereSamples[idx + 0] = x;
        m_hemisphereSamples[idx + 1] = y;
        m_hemisphereSamples[idx + 2] = z;
        m_hemisphereSamples[idx + 3] = 0.0f;
    }
}

void SSAOPass::CreateNoiseTexture() {
    if (!m_factory) return;

    std::mt19937 rng(137);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    // 4x4 噪声纹理: 16 个像素，每个 RGBA 包含随机 3D 向量
    constexpr uint32_t noiseSize = 4;
    uint8_t noiseData[noiseSize * noiseSize * 4]; // RGBA8

    for (uint32_t i = 0; i < noiseSize * noiseSize; i++) {
        float rx = dist(rng);
        float ry = dist(rng);
        float rz = dist(rng);

        // 归一化并在半球内
        float len = sqrt(rx * rx + ry * ry + rz * rz);
        if (len > 0.001f) {
            rx /= len;
            ry /= len;
            rz /= len;
        }

        noiseData[i * 4 + 0] = static_cast<uint8_t>((rx * 0.5f + 0.5f) * 255.0f);
        noiseData[i * 4 + 1] = static_cast<uint8_t>((ry * 0.5f + 0.5f) * 255.0f);
        noiseData[i * 4 + 2] = static_cast<uint8_t>((rz * 0.5f + 0.5f) * 255.0f);
        noiseData[i * 4 + 3] = 255;
    }

    TextureDesc noiseDesc;
    noiseDesc.type = TextureType::Texture2D;
    noiseDesc.format = TextureFormat::RGBA8_UNorm;
    noiseDesc.width = noiseSize;
    noiseDesc.height = noiseSize;
    noiseDesc.depth = 1;
    noiseDesc.mipLevels = 1;
    noiseDesc.arraySize = 1;
    noiseDesc.allowRenderTarget = false;
    noiseDesc.allowUnorderedAccess = false;
    noiseDesc.allowShaderResource = true;
    noiseDesc.initialData = noiseData;
    noiseDesc.dataSize = sizeof(noiseData);

    m_noiseTexture.reset(m_factory->CreateTextureImpl(noiseDesc).release());
}

} // namespace Prisma::Graphic
