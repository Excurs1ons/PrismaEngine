#include "MLGIProbeUpdatePass.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "app/Engine.h"
#include "Logger.h"

#include <cstring>

namespace Prisma {

MLGIProbeUpdatePass::MLGIProbeUpdatePass() = default;

MLGIProbeUpdatePass::~MLGIProbeUpdatePass() {
    Cleanup();
}

bool MLGIProbeUpdatePass::Setup(Graphic::IRenderDevice* device,
                                 const ProbeGrid& grid,
                                 uint32_t raysPerProbe) {
    if (!device) {
        LOG_ERROR("MLGIProbeUpdatePass", "Setup: null device");
        return false;
    }
    m_device = device;
    m_factory = device->GetResourceFactory();
    if (!m_factory) {
        LOG_ERROR("MLGIProbeUpdatePass", "Setup: null resource factory");
        return false;
    }

    m_probeGrid = grid;
    m_raysPerProbe = std::min(raysPerProbe, 32u);

    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rm) {
        LOG_ERROR("MLGIProbeUpdatePass", "Setup: null render resource manager");
        return false;
    }

    m_probeUpdateShader = rm->LoadShaderSync("assets/shaders/mlgi_probe_update.comp.spv");
    if (!m_probeUpdateShader) {
        LOG_WARN("MLGIProbeUpdatePass", "mlgi_probe_update.comp.spv not found, probe update pass disabled");
        return false;
    }

    if (!CreatePipeline(device)) {
        LOG_ERROR("MLGIProbeUpdatePass", "Failed to create compute pipeline");
        return false;
    }

    if (!CreateDescriptorSets(device)) {
        LOG_ERROR("MLGIProbeUpdatePass", "Failed to create descriptor sets");
        return false;
    }

    m_ready = true;
    LOG_INFO("MLGIProbeUpdatePass", "Initialized: {} probes, {} rays/probe",
             m_probeGrid.probeCount, m_raysPerProbe);
    return true;
}

void MLGIProbeUpdatePass::Cleanup() {
    m_probeUpdatePipeline.reset();
    m_probeUpdateShader.reset();
    m_descSet.reset();
    m_descSetLayout.reset();
    m_probeGridUBO.reset();
    m_ready = false;
    m_device = nullptr;
    m_factory = nullptr;
}

void MLGIProbeUpdatePass::Execute(Graphic::ICommandBuffer* cmd,
                                   Graphic::IBuffer* probeSamplesBuffer,
                                   void* tlasHandle,
                                   Graphic::IBuffer* sceneSSBO,
                                   Graphic::IBuffer* outputSH) {
    if (!m_ready || !cmd || !probeSamplesBuffer || !tlasHandle || !sceneSSBO || !outputSH) {
        LOG_WARN("MLGIProbeUpdatePass", "Execute: invalid args or not ready");
        return;
    }

    if (m_probeGridUBO) {
        ProbeGridUBOData ubo{};
        ubo.gridOrigin   = glm::vec4(m_probeGrid.origin, 0.0f);
        ubo.probeSpacing = m_probeGrid.spacing.x;
        ubo.gridDim      = glm::ivec4(m_probeGrid.dimensions, 0);
        ubo.raysPerProbe = static_cast<int32_t>(m_raysPerProbe);
        ubo.maxTraceDist = 1000.0f;
        ubo.skyColor     = glm::vec4(0.53f, 0.81f, 0.98f, 0.0f);
        m_probeGridUBO->UpdateData(&ubo, sizeof(ubo), 0);
    }

    m_descSet->BindBuffer(0, m_probeGridUBO.get(), 0, m_probeGridUBO->GetSize(),
                          Graphic::DescriptorType::UniformBuffer);
    m_descSet->BindBuffer(1, probeSamplesBuffer, 0, probeSamplesBuffer->GetSize(),
                          Graphic::DescriptorType::StorageBuffer);
    m_descSet->BindAccelerationStructure(2, tlasHandle);
    m_descSet->BindBuffer(3, sceneSSBO, 0, sceneSSBO->GetSize(),
                          Graphic::DescriptorType::StorageBuffer);
    m_descSet->BindBuffer(4, outputSH, 0, outputSH->GetSize(),
                          Graphic::DescriptorType::StorageBuffer);
    m_descSet->Update();

    cmd->SetComputePipeline(m_probeUpdatePipeline.get());
    cmd->BindDescriptorSet(0, m_descSet.get());

    uint32_t probeCount = m_probeGrid.probeCount;
    if (probeCount > 0) {
        cmd->Dispatch(probeCount, 1, 1);
    }
}

bool MLGIProbeUpdatePass::CreatePipeline(Graphic::IRenderDevice* device) {
    if (!m_factory || !device) {
        return false;
    }

    m_probeUpdatePipeline.reset(m_factory->CreateComputePipelineImpl().release());
    m_probeUpdatePipeline->SetShader(m_probeUpdateShader);
    m_probeUpdatePipeline->SetPushConstantRange(0);
    if (!m_probeUpdatePipeline->Create(device)) {
        LOG_ERROR("MLGIProbeUpdatePass", "Failed to create compute pipeline");
        return false;
    }

    return true;
}

bool MLGIProbeUpdatePass::CreateDescriptorSets(Graphic::IRenderDevice* device) {
    if (!m_factory) {
        return false;
    }

    auto layouts = m_probeUpdatePipeline->GetDescriptorSetLayouts();
    if (layouts.empty()) {
        LOG_ERROR("MLGIProbeUpdatePass", "No descriptor set layouts from pipeline");
        return false;
    }

    m_descSetLayout = layouts[0];
    m_descSet = m_factory->CreateDescriptorSet(m_descSetLayout.get());
    if (!m_descSet) {
        LOG_ERROR("MLGIProbeUpdatePass", "Failed to create descriptor set");
        return false;
    }

    {
        Graphic::BufferDesc desc{};
        desc.type  = Graphic::BufferType::Uniform;
        desc.size  = sizeof(ProbeGridUBOData);
        desc.stride = 0;
        desc.usage = Graphic::BufferUsage::ShaderResource
                   | Graphic::BufferUsage::Dynamic;
        desc.name  = "MLGI_ProbeGridUBO_Update";
        m_probeGridUBO = m_factory->CreateBufferImpl(desc);
        if (!m_probeGridUBO) {
            LOG_ERROR("MLGIProbeUpdatePass", "Failed to create probe grid UBO");
            return false;
        }
    }

    m_descSet->BindBuffer(0, nullptr, 0, 0, Graphic::DescriptorType::UniformBuffer);
    m_descSet->BindBuffer(1, nullptr, 0, 0, Graphic::DescriptorType::StorageBuffer);
    m_descSet->BindBuffer(3, nullptr, 0, 0, Graphic::DescriptorType::StorageBuffer);
    m_descSet->BindBuffer(4, nullptr, 0, 0, Graphic::DescriptorType::StorageBuffer);
    m_descSet->Update();

    return true;
}

} // namespace Prisma
