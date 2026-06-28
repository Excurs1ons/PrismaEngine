#include "MLGITemporalPass.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "app/Engine.h"
#include "Logger.h"

namespace Prisma {

MLGITemporalPass::MLGITemporalPass() = default;

MLGITemporalPass::~MLGITemporalPass() {
    Cleanup();
}

bool MLGITemporalPass::Setup(Graphic::IRenderDevice* device) {
    if (!device) {
        LOG_ERROR("MLGITemporalPass", "Setup: null device");
        return false;
    }
    m_device = device;
    m_factory = device->GetResourceFactory();
    if (!m_factory) {
        LOG_ERROR("MLGITemporalPass", "Setup: null resource factory");
        return false;
    }

    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rm) {
        LOG_ERROR("MLGITemporalPass", "Setup: null render resource manager");
        return false;
    }

    m_temporalShader = rm->LoadShaderSync("assets/shaders/mlgi_temporal.comp.spv");
    if (!m_temporalShader) {
        LOG_WARN("MLGITemporalPass", "mlgi_temporal.comp.spv not found, temporal pass disabled");
        return false;
    }

    if (!CreatePipelines()) {
        LOG_ERROR("MLGITemporalPass", "Failed to create compute pipeline");
        return false;
    }

    if (!CreateDescriptorSets()) {
        LOG_ERROR("MLGITemporalPass", "Failed to create descriptor sets");
        return false;
    }

    m_ready = true;
    LOG_INFO("MLGITemporalPass", "Initialized successfully");
    return true;
}

void MLGITemporalPass::Execute(Graphic::ICommandBuffer* cmd,
                                Graphic::IBuffer* currentSH,
                                Graphic::IBuffer* historySH,
                                Graphic::IBuffer* blendedSH,
                                uint32_t totalCoeffCount,
                                float blendFactor,
                                uint32_t temporalFrame,
                                bool reset) {
    if (!m_ready || !cmd || !currentSH || !historySH || !blendedSH) {
        LOG_WARN("MLGITemporalPass", "Execute: invalid args or not ready");
        return;
    }
    if (totalCoeffCount == 0) {
        return;
    }

    m_descSet->BindBuffer(0, currentSH, 0, currentSH->GetSize(),
                          Graphic::DescriptorType::StorageBuffer);
    m_descSet->BindBuffer(1, historySH, 0, historySH->GetSize(),
                          Graphic::DescriptorType::StorageBuffer);
    m_descSet->BindBuffer(2, blendedSH, 0, blendedSH->GetSize(),
                          Graphic::DescriptorType::StorageBuffer);
    m_descSet->Update();

    cmd->SetComputePipeline(m_temporalPipeline.get());
    cmd->BindDescriptorSet(0, m_descSet.get());

    struct TemporalPushConstants {
        float  blendFactor;
        uint32_t totalCoeffCount;
        uint32_t temporalFrame;
        uint32_t resetFlag;
    } pc = {};
    pc.blendFactor      = blendFactor;
    pc.totalCoeffCount  = totalCoeffCount;
    pc.temporalFrame    = temporalFrame;
    pc.resetFlag        = reset ? 1u : 0u;

    cmd->PushConstants(Graphic::ShaderType::Compute, &pc, sizeof(pc));

    uint32_t groups = (totalCoeffCount + 255) / 256;
    cmd->Dispatch(std::max(groups, 1u), 1, 1);
}

void MLGITemporalPass::CopyBack(Graphic::ICommandBuffer* cmd,
                                 Graphic::IBuffer* blendedSH,
                                 Graphic::IBuffer* historySH,
                                 uint32_t totalCoeffCount) {
    if (!m_ready || !cmd || !blendedSH || !historySH) {
        LOG_WARN("MLGITemporalPass", "CopyBack: invalid args or not ready");
        return;
    }
    if (totalCoeffCount == 0) {
        return;
    }

    m_descSet->BindBuffer(0, blendedSH, 0, blendedSH->GetSize(),
                          Graphic::DescriptorType::StorageBuffer);
    m_descSet->BindBuffer(1, blendedSH, 0, blendedSH->GetSize(),
                          Graphic::DescriptorType::StorageBuffer);
    m_descSet->BindBuffer(2, historySH, 0, historySH->GetSize(),
                          Graphic::DescriptorType::StorageBuffer);
    m_descSet->Update();

    cmd->SetComputePipeline(m_temporalPipeline.get());
    cmd->BindDescriptorSet(0, m_descSet.get());

    struct TemporalPushConstants {
        float  blendFactor;
        uint32_t totalCoeffCount;
        uint32_t temporalFrame;
        uint32_t resetFlag;
    } pc = {};
    pc.blendFactor      = 0.0f;
    pc.totalCoeffCount  = totalCoeffCount;
    pc.temporalFrame    = 0u;
    pc.resetFlag        = 0u;

    cmd->PushConstants(Graphic::ShaderType::Compute, &pc, sizeof(pc));

    uint32_t groups = (totalCoeffCount + 255) / 256;
    cmd->Dispatch(std::max(groups, 1u), 1, 1);
}

void MLGITemporalPass::Cleanup() {
    m_temporalPipeline.reset();
    m_temporalShader.reset();
    m_descSet.reset();
    m_descSetLayout.reset();
    m_ready = false;
    m_device = nullptr;
    m_factory = nullptr;
}

bool MLGITemporalPass::CreatePipelines() {
    if (!m_factory || !m_device) {
        return false;
    }

    m_temporalPipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_temporalPipeline->SetShader(m_temporalShader);
    m_temporalPipeline->SetPushConstantRange(sizeof(float) + 3 * sizeof(uint32_t));
    if (!m_temporalPipeline->Create(m_device)) {
        LOG_ERROR("MLGITemporalPass", "Failed to create compute pipeline");
        return false;
    }

    return true;
}

bool MLGITemporalPass::CreateDescriptorSets() {
    if (!m_factory) {
        return false;
    }

    auto layouts = m_temporalPipeline->GetDescriptorSetLayouts();
    if (layouts.empty()) {
        LOG_ERROR("MLGITemporalPass", "No descriptor set layouts from pipeline");
        return false;
    }

    m_descSetLayout = layouts[0];
    m_descSet = m_factory->CreateDescriptorSet(m_descSetLayout.get());
    if (!m_descSet) {
        LOG_ERROR("MLGITemporalPass", "Failed to create descriptor set");
        return false;
    }

    // Pre-bind nulls
    m_descSet->BindBuffer(0, nullptr, 0, 0, Graphic::DescriptorType::StorageBuffer);
    m_descSet->BindBuffer(1, nullptr, 0, 0, Graphic::DescriptorType::StorageBuffer);
    m_descSet->BindBuffer(2, nullptr, 0, 0, Graphic::DescriptorType::StorageBuffer);
    m_descSet->Update();

    return true;
}

} // namespace Prisma
