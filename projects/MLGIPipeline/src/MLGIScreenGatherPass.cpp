#include "MLGIScreenGatherPass.h"
#include "app/Engine.h"
#include "Logger.h"

namespace Prisma {

MLGIScreenGatherPass::MLGIScreenGatherPass() = default;
MLGIScreenGatherPass::~MLGIScreenGatherPass() { Shutdown(); }

bool MLGIScreenGatherPass::Initialize(Graphic::IRenderDevice* device,
                                       uint32_t probeCount,
                                       uint32_t width,
                                       uint32_t height) {
    if (!device) {
        LOG_ERROR("MLGIScreenGatherPass", "Initialize: device is null");
        return false;
    }

    m_width  = width;
    m_height = height;

    if (!CreatePipeline(device)) {
        LOG_ERROR("MLGIScreenGatherPass", "Failed to create compute pipeline");
        return false;
    }

    if (!CreateDescriptorSet(device)) {
        LOG_ERROR("MLGIScreenGatherPass", "Failed to create descriptor set");
        return false;
    }

    m_initialized = true;
    LOG_INFO("MLGIScreenGatherPass", "Initialized: {}x{} output, {} probes",
             width, height, probeCount);
    return true;
}

void MLGIScreenGatherPass::Shutdown() {
    if (!m_initialized) return;

    m_pipeline.reset();
    m_screenGatherShader.reset();
    m_descriptorSet.reset();
    m_probeGridUBO.reset();
    m_cameraUBO.reset();
    m_initialized = false;
    m_enabled = true;

    LOG_INFO("MLGIScreenGatherPass", "Shutdown complete");
}

bool MLGIScreenGatherPass::CreatePipeline(Graphic::IRenderDevice* device) {
    auto* factory = device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("MLGIScreenGatherPass", "No resource factory available");
        return false;
    }

    // Create compute pipeline
    m_pipeline = factory->CreateComputePipelineImpl();
    if (!m_pipeline) {
        LOG_ERROR("MLGIScreenGatherPass", "Failed to create compute pipeline object");
        return false;
    }

    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rm) {
        LOG_ERROR("MLGIScreenGatherPass", "No render resource manager available");
        return false;
    }

    m_screenGatherShader = rm->LoadShaderSync("assets/shaders/mlgi_screen_gather.comp.spv");
    if (!m_screenGatherShader) {
        LOG_WARN("MLGIScreenGatherPass", "mlgi_screen_gather.comp.spv not found, screen gather pass disabled");
        return false;
    }

    m_pipeline->SetShader(m_screenGatherShader);

    // Set push constant range (16 bytes: giStrength, ambientFallback, useNormalMap, pad)
    m_pipeline->SetPushConstantRange(sizeof(GatherPushConstants));

    if (!m_pipeline->Create(device)) {
        LOG_ERROR("MLGIScreenGatherPass", "Failed to compile compute pipeline");
        return false;
    }

    LOG_INFO("MLGIScreenGatherPass", "Compute pipeline created successfully");
    return true;
}

bool MLGIScreenGatherPass::CreateDescriptorSet(Graphic::IRenderDevice* device) {
    auto* factory = device->GetResourceFactory();
    if (!factory || !m_pipeline) return false;

    const auto& layouts = m_pipeline->GetDescriptorSetLayouts();
    if (layouts.empty()) {
        LOG_ERROR("MLGIScreenGatherPass", "Pipeline has no descriptor set layouts");
        return false;
    }

    m_descriptorSet = factory->CreateDescriptorSet(layouts[0].get());
    if (!m_descriptorSet) {
        LOG_ERROR("MLGIScreenGatherPass", "Failed to create descriptor set");
        return false;
    }

    // Create probe grid UBO (std140 layout: 3 vec4 + 1 ivec4 = 64 bytes)
    {
        Graphic::BufferDesc desc{};
        desc.type  = Graphic::BufferType::Constant;
        desc.size  = sizeof(glm::vec4) * 3 + sizeof(glm::ivec4);
        desc.stride = 0;
        desc.usage = Graphic::BufferUsage::ShaderResource
                   | Graphic::BufferUsage::Dynamic;
        desc.name  = "MLGI_ProbeGridUBO";
        m_probeGridUBO = factory->CreateBufferImpl(desc);
        if (!m_probeGridUBO) {
            LOG_ERROR("MLGIScreenGatherPass", "Failed to create probe grid UBO");
            return false;
        }
    }

    // Create camera UBO (std140: 4 vec4 + 4 floats = 80 bytes)
    {
        Graphic::BufferDesc desc{};
        desc.type  = Graphic::BufferType::Constant;
        desc.size  = sizeof(glm::vec4) * 4 + sizeof(float) * 4;
        desc.stride = 0;
        desc.usage = Graphic::BufferUsage::ShaderResource
                   | Graphic::BufferUsage::Dynamic;
        desc.name  = "MLGI_CameraUBO";
        m_cameraUBO = factory->CreateBufferImpl(desc);
        if (!m_cameraUBO) {
            LOG_ERROR("MLGIScreenGatherPass", "Failed to create camera UBO");
            return false;
        }
    }

    // Bind probe grid UBO to binding 3
    m_descriptorSet->BindBuffer(3, m_probeGridUBO.get(), 0,
                                sizeof(glm::vec4) * 3 + sizeof(glm::ivec4),
                                Graphic::DescriptorType::UniformBuffer);

    // Bind camera UBO to binding 4
    m_descriptorSet->BindBuffer(4, m_cameraUBO.get(), 0,
                                sizeof(glm::vec4) * 4 + sizeof(float) * 4,
                                Graphic::DescriptorType::UniformBuffer);

    m_descriptorSet->Update();

    LOG_INFO("MLGIScreenGatherPass", "Descriptor set created with UBO bindings");
    return true;
}

void MLGIScreenGatherPass::Dispatch(Graphic::ICommandBuffer* cmd,
                                     Graphic::IBuffer* blendedSHBuffer,
                                     Graphic::ITexture* depthTexture,
                                     Graphic::ITexture* normalTexture,
                                     Graphic::ITexture* giOutputTexture,
                                     float giStrength,
                                     bool useNormalMap) {
    if (!m_initialized || !m_enabled || !cmd) {
        return;
    }

    if (!blendedSHBuffer || !depthTexture || !giOutputTexture) {
        LOG_WARN("MLGIScreenGatherPass", "Dispatch: missing required resources");
        return;
    }

    // Barrier: ensure SH buffer is readable and output image is writable
    cmd->PipelineBarrier();

    // Bind compute pipeline
    cmd->SetComputePipeline(m_pipeline.get());

    // Bind descriptor set (set 0)
    cmd->BindDescriptorSet(0, m_descriptorSet.get());

    // Update SH buffer binding (binding 2) — this changes every frame
    m_descriptorSet->BindBuffer(2, blendedSHBuffer, 0, 0,
                                Graphic::DescriptorType::StorageBuffer);

    // Bind depth texture (binding 1)
    m_descriptorSet->BindTexture(1, depthTexture, nullptr);

    // Bind normal texture (binding 5) or a default white texture
    if (normalTexture && useNormalMap) {
        m_descriptorSet->BindTexture(5, normalTexture, nullptr);
    }

    // Bind output storage image (binding 0)
    m_descriptorSet->BindStorageImage(0, giOutputTexture);

    m_descriptorSet->Update();

    // Push constants
    GatherPushConstants pc{};
    pc.giStrength      = giStrength;
    pc.ambientFallback = 0.02f;
    pc.useNormalMap    = useNormalMap ? 1 : 0;
    pc._pad0           = 0;
    cmd->PushConstants(Graphic::ShaderType::Compute, &pc, sizeof(pc));

    // Dispatch: 8x8 thread groups
    uint32_t gx = (m_width  + 7) / 8;
    uint32_t gy = (m_height + 7) / 8;
    cmd->Dispatch(gx, gy, 1);
}

void MLGIScreenGatherPass::SetProbeGrid(const ProbeGrid& grid) {
    m_probeGrid = grid;

    if (m_probeGridUBO) {
        // Pack data in std140 layout:
        //   vec4 gridOrigin  (16 bytes)
        //   vec4 gridSpacing (16 bytes)
        //   ivec4 gridDim    (16 bytes) — .w = total probe count
        struct {
            glm::vec4 origin;
            glm::vec4 spacing;
            glm::ivec4 dim;
        } uboData;

        uboData.origin  = glm::vec4(grid.origin, 0.0f);
        uboData.spacing = glm::vec4(grid.spacing, 0.0f);
        uboData.dim     = glm::ivec4(grid.dimensions, static_cast<int>(grid.probeCount));

        m_probeGridUBO->UpdateData(&uboData, sizeof(uboData), 0);
    }
}

void MLGIScreenGatherPass::Resize(uint32_t width, uint32_t height) {
    m_width  = width;
    m_height = height;
    LOG_INFO("MLGIScreenGatherPass", "Resized to {}x{}", width, height);
}

} // namespace Prisma
