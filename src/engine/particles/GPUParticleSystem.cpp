#include "GPUParticleSystem.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/IComputePipeline.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/ISwapChain.h"
#include "graphic/ICamera.h"
#include "graphic/RenderDesc.h"
#include "app/Engine.h"
#include "Logger.h"
#include <cstring>
#include <random>
#include <unordered_map>

namespace Prisma::Particles {

// ============================================================================
// Uniform 数据结构（匹配 GLSL std140 布局）
// ============================================================================

// particles_update.comp 的 uniform 数据
// layout(std140, binding = 1) uniform ParticleUniforms { ... }
struct ParticleUniformData {
    float deltaTime;
    float gravityX;
    float gravityY;
    float gravityZ;
    uint32_t aliveCount;
    uint32_t maxParticles;
    float padding0;
    float padding1;
};
static_assert(sizeof(ParticleUniformData) == 32, "ParticleUniformData must be 32 bytes (std140)");

// particles_render.vert 的 camera uniform 数据
// layout(std140, binding = 1) uniform CameraUniforms { ... }
// 着色器使用 GL_EXT_scalar_block_layout，vec3 按标量对齐（4 字节）
// 因此 u_Near(offset96) + u_CameraForward(offset100) + u_Far(offset112) + u_Use2D(offset116)
// 共计 120 字节，对齐到 16 → 128 字节
struct alignas(16) CameraUniformPacked {
    float viewProj[16];     // offset 0:   mat4 u_ViewProjection (4*vec4)
    float cameraRight[4];   // offset 64:  vec3 u_CameraRight + 1 pad
    float cameraUp[4];      // offset 80:  vec3 u_CameraUp + 1 pad
    float nearFwd[4];       // offset 96:  float u_Near + vec3 u_CameraForward
    float farUse2D[4];      // offset 112: float u_Far + float u_Use2D + 2 pad
};
static_assert(sizeof(CameraUniformPacked) == 128, "CameraUniformPacked must be 128 bytes (std140)");

// ============================================================================
// GPU 资源辅助存储（不修改头文件，通过 per-instance map 管理）
// 计算管线与图形管线有不同的描述符集布局，需要各自独立的 DescriptorSet
// ============================================================================

struct GPUParticleExtraResources {
    std::shared_ptr<Graphic::IDescriptorSet> gfxDescriptorSet;
    std::shared_ptr<Graphic::IDescriptorSetLayout> gfxDescriptorSetLayout;
    std::shared_ptr<Graphic::IBuffer> cameraUniformBuffer;
    std::shared_ptr<Graphic::IBuffer> counterBuffer;
};

static std::unordered_map<const GPUParticleSystem*, GPUParticleExtraResources> s_Extra;

static GPUParticleExtraResources& GetExtra(const GPUParticleSystem* sys) {
    return s_Extra[sys];
}

static void RemoveExtra(const GPUParticleSystem* sys) {
    s_Extra.erase(sys);
}

// ============================================================================
// 构造函数 / 析构函数
// ============================================================================

GPUParticleSystem::GPUParticleSystem()
    : m_RNG(std::random_device{}())
{
}

GPUParticleSystem::~GPUParticleSystem() {
    Shutdown();
}

// ============================================================================
// Initialize — 创建所有 GPU 资源
// ============================================================================

bool GPUParticleSystem::Initialize(Graphic::IRenderDevice* device, uint32_t maxParticles) {
    if (m_Initialized) return true;
    if (!device) return false;

    m_Device = device;
    m_MaxParticles = std::max(maxParticles, 1u);
    m_AliveCount = 0;
    m_FreeHead = 0;
    m_SpawnAccumulator = 0.0f;
    m_ParticleCPUData.clear();

    // 1. 创建所有缓冲区
    if (!CreateBuffers()) {
        LOG_ERROR("GPUParticleSystem", "Failed to create GPU buffers");
        Shutdown();
        return false;
    }

    // 2. 加载着色器
    if (!CreateShaders()) {
        LOG_ERROR("GPUParticleSystem", "Failed to load shaders");
        Shutdown();
        return false;
    }

    // 3. 创建管线
    if (!CreatePipelines()) {
        LOG_ERROR("GPUParticleSystem", "Failed to create pipelines");
        Shutdown();
        return false;
    }

    // 4. 初始化粒子数据（全标记为死亡）
    {
        std::vector<GPUParticleData> deadInit(m_MaxParticles);
        for (auto& p : deadInit) {
            p.life.w = -1.0f; // 死亡标记
        }
        m_ParticleBuffer->UpdateData(deadInit.data(), deadInit.size() * sizeof(GPUParticleData), 0);
    }

    // 5. 初始化计数器
    {
        uint32_t initCounters[2] = {0, m_MaxParticles};
        auto& extra = GetExtra(this);
        extra.counterBuffer->UpdateData(initCounters, sizeof(initCounters), 0);
    }

    m_Initialized = true;
    LOG_INFO("GPUParticleSystem", "Initialized with max {} particles, pipeline={}",
             m_MaxParticles, "GPU Compute");
    return true;
}

// ============================================================================
// Shutdown — 释放所有 GPU 资源
// ============================================================================

void GPUParticleSystem::Shutdown() {
    if (!m_Initialized && !m_Device) return;

    // 等待 GPU 完成
    if (m_Device) {
        m_Device->WaitForIdle();
    }

    // 释放 GPU 资源（shared_ptr reset）
    m_ParticleBuffer.reset();
    m_IndirectBuffer.reset();
    m_StagingBuffer.reset();
    m_UniformBuffer.reset();

    m_ComputePipeline.reset();
    m_GraphicsPipeline.reset();
    m_DescriptorSet.reset();
    m_DescriptorSetLayout.reset();

    m_ComputeShader.reset();
    m_VertexShader.reset();
    m_FragmentShader.reset();

    // 释放额外 GPU 资源
    auto& extra = GetExtra(this);
    extra.gfxDescriptorSet.reset();
    extra.gfxDescriptorSetLayout.reset();
    extra.cameraUniformBuffer.reset();
    extra.counterBuffer.reset();
    RemoveExtra(this);

    m_ParticleCPUData.clear();
    m_ParticleCPUData.shrink_to_fit();

    m_Device = nullptr;
    m_Initialized = false;
    m_AliveCount = 0;
    m_FreeHead = 0;

    LOG_INFO("GPUParticleSystem", "Shutdown complete");
}

// ============================================================================
// CreateBuffers — 创建所有 GPU 缓冲区
// ============================================================================

bool GPUParticleSystem::CreateBuffers() {
    auto* factory = m_Device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("GPUParticleSystem", "No resource factory available");
        return false;
    }

    // 1. 粒子 SSBO（GPU 读写）
    {
        Graphic::BufferDesc desc;
        desc.name = "GPUParticle_Buffer";
        desc.type = Graphic::BufferType::Structured;
        desc.size = m_MaxParticles * sizeof(GPUParticleData);
        desc.stride = sizeof(GPUParticleData);
        desc.usage = Graphic::BufferUsage::UnorderedAccess | Graphic::BufferUsage::ShaderResource;
        m_ParticleBuffer.reset(factory->CreateBufferImpl(desc).release());
        if (!m_ParticleBuffer) {
            LOG_ERROR("GPUParticleSystem", "Failed to create particle SSBO");
            return false;
        }
    }

    // 2. Uniform buffer（每帧更新的粒子参数）
    {
        Graphic::BufferDesc desc;
        desc.name = "GPUParticle_Uniforms";
        desc.type = Graphic::BufferType::Constant;
        desc.size = sizeof(ParticleUniformData);
        desc.usage = Graphic::BufferUsage::Dynamic;
        m_UniformBuffer.reset(factory->CreateBufferImpl(desc).release());
        if (!m_UniformBuffer) {
            LOG_ERROR("GPUParticleSystem", "Failed to create uniform buffer");
            return false;
        }
    }

    // 3. 摄像机 Uniform buffer（图形管线使用）
    auto& extra = GetExtra(this);
    {
        Graphic::BufferDesc desc;
        desc.name = "GPUParticle_CameraUBO";
        desc.type = Graphic::BufferType::Constant;
        desc.size = sizeof(CameraUniformPacked);
        desc.usage = Graphic::BufferUsage::Dynamic;
        extra.cameraUniformBuffer.reset(factory->CreateBufferImpl(desc).release());
        if (!extra.cameraUniformBuffer) {
            LOG_ERROR("GPUParticleSystem", "Failed to create camera uniform buffer");
            return false;
        }
    }

    // 4. 计数器缓冲区（GPU 原子计数，目前由 CPU 维护影子值）
    {
        Graphic::BufferDesc desc;
        desc.name = "GPUParticle_Counter";
        desc.type = Graphic::BufferType::Structured;
        desc.size = sizeof(uint32_t) * 2; // aliveCount, totalCount
        desc.stride = sizeof(uint32_t);
        desc.usage = Graphic::BufferUsage::UnorderedAccess | Graphic::BufferUsage::ShaderResource;
        extra.counterBuffer.reset(factory->CreateBufferImpl(desc).release());
        if (!extra.counterBuffer) {
            LOG_ERROR("GPUParticleSystem", "Failed to create counter buffer");
            return false;
        }
    }

    // 5. 间接绘制缓冲区（为 GPU-driven pipeline 预留）
    {
        Graphic::BufferDesc desc;
        desc.name = "GPUParticle_Indirect";
        desc.type = Graphic::BufferType::IndirectArgument;
        desc.size = sizeof(ParticleDrawIndirect);
        desc.usage = Graphic::BufferUsage::Default;
        m_IndirectBuffer.reset(factory->CreateBufferImpl(desc).release());
        if (!m_IndirectBuffer) {
            LOG_ERROR("GPUParticleSystem", "Failed to create indirect buffer");
            return false;
        }
    }

    // 6. Staging 缓冲区（CPU→GPU 上传）
    {
        Graphic::BufferDesc desc;
        desc.name = "GPUParticle_Staging";
        desc.type = Graphic::BufferType::Structured;
        desc.size = m_MaxParticles * sizeof(GPUParticleData);
        desc.stride = sizeof(GPUParticleData);
        desc.usage = Graphic::BufferUsage::Upload;
        m_StagingBuffer.reset(factory->CreateBufferImpl(desc).release());
        if (!m_StagingBuffer) {
            LOG_ERROR("GPUParticleSystem", "Failed to create staging buffer");
            return false;
        }
    }

    return true;
}

// ============================================================================
// CreateShaders — 加载粒子着色器 SPIR-V
// ============================================================================

bool GPUParticleSystem::CreateShaders() {
    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rm) {
        LOG_ERROR("GPUParticleSystem", "No render resource manager available");
        return false;
    }

    // 计算着色器
    m_ComputeShader = rm->LoadShaderSync("resources/common/shaders/glsl/particles/particles_update.comp.spv");
    if (!m_ComputeShader) {
        LOG_WARN("GPUParticleSystem", "Compute shader not found, trying fallback path");
        m_ComputeShader = rm->LoadShaderSync("shaders/glsl/particles/particles_update.comp.spv");
    }
    if (!m_ComputeShader) {
        LOG_WARN("GPUParticleSystem", "particles_update.comp.spv not found, GPU particles disabled");
        return false;
    }

    // 顶点着色器
    m_VertexShader = rm->LoadShaderSync("resources/common/shaders/glsl/particles/particles_render.vert.spv");
    if (!m_VertexShader) {
        m_VertexShader = rm->LoadShaderSync("shaders/glsl/particles/particles_render.vert.spv");
    }
    if (!m_VertexShader) {
        LOG_WARN("GPUParticleSystem", "particles_render.vert.spv not found");
        return false;
    }

    // 片段着色器
    m_FragmentShader = rm->LoadShaderSync("resources/common/shaders/glsl/particles/particles_render.frag.spv");
    if (!m_FragmentShader) {
        m_FragmentShader = rm->LoadShaderSync("shaders/glsl/particles/particles_render.frag.spv");
    }
    if (!m_FragmentShader) {
        LOG_WARN("GPUParticleSystem", "particles_render.frag.spv not found");
        return false;
    }

    return true;
}

// ============================================================================
// CreatePipelines — 创建计算管道 + 图形管道 + 描述符集
// ============================================================================

bool GPUParticleSystem::CreatePipelines() {
    auto* factory = m_Device->GetResourceFactory();
    if (!factory) return false;

    // ============================================================
    // 1. 计算管线（粒子物理更新）
    // ============================================================
    m_ComputePipeline.reset(factory->CreateComputePipelineImpl().release());
    m_ComputePipeline->SetShader(m_ComputeShader);
    // 计算着色器不使用 push constants，尺寸设为 0
    m_ComputePipeline->SetPushConstantRange(0);
    if (!m_ComputePipeline->Create(m_Device)) {
        LOG_ERROR("GPUParticleSystem", "Failed to create compute pipeline");
        return false;
    }

    // 从计算管线获取描述符集布局（含 binding 0,1,2,3）
    auto compLayouts = m_ComputePipeline->GetDescriptorSetLayouts();
    if (compLayouts.empty()) {
        LOG_ERROR("GPUParticleSystem", "Compute pipeline has no descriptor set layouts");
        return false;
    }

    m_DescriptorSetLayout = compLayouts[0];
    m_DescriptorSet = factory->CreateDescriptorSet(m_DescriptorSetLayout.get());
    if (!m_DescriptorSet) {
        LOG_ERROR("GPUParticleSystem", "Failed to create compute descriptor set");
        return false;
    }

    // 绑定计算描述符集资源
    // binding 0: 粒子 SSBO (StorageBuffer)
    m_DescriptorSet->BindBuffer(0, m_ParticleBuffer.get(), 0, m_MaxParticles * sizeof(GPUParticleData),
                                Graphic::DescriptorType::StorageBuffer);
    // binding 1: 粒子 Uniform (UniformBuffer)
    m_DescriptorSet->BindBuffer(1, m_UniformBuffer.get(), 0, sizeof(ParticleUniformData),
                                Graphic::DescriptorType::UniformBuffer);
    // binding 2: 计数器 Buffer (StorageBuffer)
    auto& extra = GetExtra(this);
    m_DescriptorSet->BindBuffer(2, extra.counterBuffer.get(), 0, sizeof(uint32_t) * 2,
                                Graphic::DescriptorType::StorageBuffer);
    // binding 3: 间接 Buffer (StorageBuffer)
    m_DescriptorSet->BindBuffer(3, m_IndirectBuffer.get(), 0, sizeof(ParticleDrawIndirect),
                                Graphic::DescriptorType::StorageBuffer);
    m_DescriptorSet->Update();

    // ============================================================
    // 2. 图形管线（粒子渲染）
    // ============================================================
    m_GraphicsPipeline.reset(factory->CreatePipelineStateImpl().release());

    // 设置着色器
    m_GraphicsPipeline->SetShader(Graphic::ShaderType::Vertex, m_VertexShader);
    m_GraphicsPipeline->SetShader(Graphic::ShaderType::Pixel, m_FragmentShader);

    // 图元拓扑：TriangleList（每 3 个顶点一个三角形，4 顶点 quad = 2 个三角形）
    // 注意：顶点着色器使用 gl_VertexIndex / 4 索引粒子，因此需要 TriangleList
    m_GraphicsPipeline->SetPrimitiveTopology(Graphic::PrimitiveTopology::TriangleList);

    // 无顶点输入 — 粒子数据从 SSBO 读取
    m_GraphicsPipeline->SetInputLayout({});

    // 渲染目标格式（从交换链获取）
    Graphic::TextureFormat rtFormat = Graphic::TextureFormat::RGBA8_UNorm;
    Graphic::TextureFormat dsFormat = Graphic::TextureFormat::D32_Float;
    if (auto* swapchain = m_Device->GetSwapChain()) {
        rtFormat = swapchain->GetFormat();
    }
    m_GraphicsPipeline->SetRenderTargetFormats({rtFormat});
    m_GraphicsPipeline->SetDepthStencilFormat(dsFormat);

    // 混合状态（根据 m_BlendMode 配置）
    {
        Graphic::BlendState blend;
        blend.blendEnable = true;
        if (m_BlendMode == ParticleBlendMode::Alpha) {
            blend.srcBlend = Graphic::BlendFactorType::SrcAlpha;
            blend.destBlend = Graphic::BlendFactorType::InvSrcAlpha;
            blend.srcBlendAlpha = Graphic::BlendFactorType::One;
            blend.destBlendAlpha = Graphic::BlendFactorType::Zero;
        } else {
            // Additive: src * srcAlpha + dst * 1
            blend.srcBlend = Graphic::BlendFactorType::SrcAlpha;
            blend.destBlend = Graphic::BlendFactorType::One;
            blend.srcBlendAlpha = Graphic::BlendFactorType::One;
            blend.destBlendAlpha = Graphic::BlendFactorType::One;
        }
        m_GraphicsPipeline->SetBlendState(blend);
    }

    // 光栅化状态（粒子 billboard 无需背面剔除）
    {
        Graphic::RasterizerState raster;
        raster.cullMode = Graphic::CullMode::None;
        raster.fillMode = Graphic::FillMode::Solid;
        raster.depthClipEnable = true;
        m_GraphicsPipeline->SetRasterizerState(raster);
    }

    // 深度模板状态（粒子不写深度）
    {
        Graphic::DepthStencilState depth;
        depth.depthEnable = true;
        depth.depthWriteEnable = false;
        depth.depthFunc = Graphic::ComparisonFunc::Less;
        m_GraphicsPipeline->SetDepthStencilState(depth);
    }

    if (!m_GraphicsPipeline->Create(m_Device)) {
        LOG_ERROR("GPUParticleSystem", "Failed to create graphics pipeline");
        return false;
    }

    // 从图形管线获取描述符集布局（含 binding 0,1）
    auto gfxLayouts = m_GraphicsPipeline->GetDescriptorSetLayouts();
    if (gfxLayouts.empty()) {
        LOG_ERROR("GPUParticleSystem", "Graphics pipeline has no descriptor set layouts");
        return false;
    }

    extra.gfxDescriptorSetLayout = gfxLayouts[0];
    extra.gfxDescriptorSet = factory->CreateDescriptorSet(extra.gfxDescriptorSetLayout.get());
    if (!extra.gfxDescriptorSet) {
        LOG_ERROR("GPUParticleSystem", "Failed to create graphics descriptor set");
        return false;
    }

    // 绑定图形描述符集资源
    // binding 0: 粒子 SSBO（只读，但仍绑定为 StorageBuffer 以匹配着色器声明）
    extra.gfxDescriptorSet->BindBuffer(0, m_ParticleBuffer.get(), 0,
                                       m_MaxParticles * sizeof(GPUParticleData),
                                       Graphic::DescriptorType::StorageBuffer);
    // binding 1: 摄像机 Uniform
    extra.gfxDescriptorSet->BindBuffer(1, extra.cameraUniformBuffer.get(), 0,
                                       sizeof(CameraUniformPacked),
                                       Graphic::DescriptorType::UniformBuffer);
    extra.gfxDescriptorSet->Update();

    // 初始化间接缓冲区（全 0）
    {
        ParticleDrawIndirect indirectCmd{};
        indirectCmd.vertexCount = 0;
        indirectCmd.instanceCount = 1;
        indirectCmd.firstVertex = 0;
        indirectCmd.firstInstance = 0;
        m_IndirectBuffer->UpdateData(&indirectCmd, sizeof(indirectCmd), 0);
    }

    LOG_INFO("GPUParticleSystem", "Pipelines created successfully");
    return true;
}

// ============================================================================
// UpdateIndirectBuffer — 更新间接绘制参数
// GPU compute 暂不写入 indirect buffer，由 CPU 维护
// ============================================================================

void GPUParticleSystem::UpdateIndirectBuffer() {
    if (!m_IndirectBuffer) return;

    ParticleDrawIndirect cmd;
    cmd.vertexCount = m_AliveCount * 4;  // 每粒子 4 顶点
    cmd.instanceCount = 1;
    cmd.firstVertex = 0;
    cmd.firstInstance = 0;

    m_IndirectBuffer->UpdateData(&cmd, sizeof(cmd), 0);
}

// ============================================================================
// UploadParticleData — 将 CPU 粒子数据上传到 GPU SSBO
// ============================================================================

void GPUParticleSystem::UploadParticleData() {
    if (m_ParticleCPUData.empty() || !m_ParticleBuffer) return;

    uint32_t count = static_cast<uint32_t>(m_ParticleCPUData.size());
    uint64_t dataSize = count * sizeof(GPUParticleData);
    uint32_t startSlot = m_FreeHead;

    // 将数据上传到 staging buffer，然后复制到 SSBO
    // 使用 staging 确保正确同步（Upload 类型缓冲区可直接映射）
    {
        auto mapped = m_StagingBuffer->Map(0, dataSize);
        if (mapped.data) {
            std::memcpy(mapped.data, m_ParticleCPUData.data(), dataSize);
            m_StagingBuffer->Unmap(0, dataSize);
        }
    }

    // 处理绕回：如果粒子写入超过 buffer 末尾，分两批复制
    if (startSlot + count > m_MaxParticles) {
        uint32_t firstBatch = m_MaxParticles - startSlot;
        uint32_t secondBatch = count - firstBatch;

        // Staging → SSBO: 第一批
        m_StagingBuffer->CopyTo(m_ParticleBuffer.get(), 0,
                                startSlot * sizeof(GPUParticleData),
                                firstBatch * sizeof(GPUParticleData));

        // Staging → SSBO: 第二批（从 staging offset 处开始）
        m_StagingBuffer->CopyTo(m_ParticleBuffer.get(),
                                firstBatch * sizeof(GPUParticleData),
                                0,
                                secondBatch * sizeof(GPUParticleData));
    } else {
        // 单批复制
        m_StagingBuffer->CopyTo(m_ParticleBuffer.get(), 0,
                                startSlot * sizeof(GPUParticleData),
                                dataSize);
    }

    // 更新空闲指针和存活计数
    m_FreeHead = (startSlot + count) % m_MaxParticles;
    m_AliveCount = std::min(m_AliveCount + count, m_MaxParticles);

    // 更新间接绘制 buffer
    UpdateIndirectBuffer();

    m_ParticleCPUData.clear();
}

// ============================================================================
// Emit2D — 在 2D 坐标发射粒子（z 强制为 0）
// ============================================================================

void GPUParticleSystem::Emit2D(float x, float y, const EmitterConfig& config) {
    if (!m_Initialized) return;

    EmitterConfig adjusted = config;
    adjusted.position = Vector3(x, y, 0.0f);
    uint32_t count = std::max(1u, config.burstCount > 0 ? config.burstCount : 1u);

    // 临时保存 m_use2D 并确保启用
    bool was2D = m_use2D;
    m_use2D = true;
    SpawnParticles(count, adjusted);
    m_use2D = was2D;
}

// ============================================================================
// Update2DCameraUniforms — 更新 2D 模式摄像机 uniform
// 使用正交投影 + 固定朝向公告板（不跟踪相机朝向）
// ============================================================================

void GPUParticleSystem::Update2DCameraUniforms(Graphic::ICamera* camera,
                                               Graphic::ICommandBuffer* cmdBuffer) {
    (void)cmdBuffer;
    auto& extra = GetExtra(this);
    if (!extra.cameraUniformBuffer) return;

    CameraUniformPacked camData;
    std::memset(&camData, 0, sizeof(camData));

    // 正交视图投影矩阵（从相机获取，应为 OrthographicCamera 的 VP）
    auto vp = camera->GetViewProjectionMatrix();
    std::memcpy(camData.viewProj, &vp, sizeof(camData.viewProj));

    // 2D 固定公告板朝向：right=(1,0,0), up=(0,1,0)
    camData.cameraRight[0] = 1.0f;
    camData.cameraRight[1] = 0.0f;
    camData.cameraRight[2] = 0.0f;
    camData.cameraRight[3] = 0.0f;

    camData.cameraUp[0] = 0.0f;
    camData.cameraUp[1] = 1.0f;
    camData.cameraUp[2] = 0.0f;
    camData.cameraUp[3] = 0.0f;

    // 近平面 + 前方向（2D 模式下前方向朝屏幕内）
    camData.nearFwd[0] = camera->GetNearPlane();
    camData.nearFwd[1] = 0.0f;
    camData.nearFwd[2] = 0.0f;
    camData.nearFwd[3] = -1.0f;

    // 远平面 + 2D 标记
    camData.farUse2D[0] = camera->GetFarPlane();
    camData.farUse2D[1] = 1.0f;  // u_Use2D = 1.0
    camData.farUse2D[2] = 0.0f;
    camData.farUse2D[3] = 0.0f;

    extra.cameraUniformBuffer->UpdateData(&camData, sizeof(camData), 0);
}

// ============================================================================
// Update3DCameraUniforms — 更新 3D 模式摄像机 uniform
// 使用透视投影 + 面向相机的公告板（跟踪相机朝向）
// ============================================================================

void GPUParticleSystem::Update3DCameraUniforms(Graphic::ICamera* camera,
                                               Graphic::ICommandBuffer* cmdBuffer) {
    (void)cmdBuffer;
    auto& extra = GetExtra(this);
    if (!extra.cameraUniformBuffer) return;

    CameraUniformPacked camData;

    // 透视视图投影矩阵（从相机获取）
    auto vp = camera->GetViewProjectionMatrix();
    std::memcpy(camData.viewProj, &vp, sizeof(camData.viewProj));

    // 3D 公告板跟踪相机朝向
    auto right = camera->GetRight();
    camData.cameraRight[0] = right.x;
    camData.cameraRight[1] = right.y;
    camData.cameraRight[2] = right.z;
    camData.cameraRight[3] = 0.0f;

    auto up = camera->GetUp();
    camData.cameraUp[0] = up.x;
    camData.cameraUp[1] = up.y;
    camData.cameraUp[2] = up.z;
    camData.cameraUp[3] = 0.0f;

    auto fwd = camera->GetForward();
    camData.nearFwd[0] = camera->GetNearPlane();
    camData.nearFwd[1] = fwd.x;
    camData.nearFwd[2] = fwd.y;
    camData.nearFwd[3] = fwd.z;

    camData.farUse2D[0] = camera->GetFarPlane();
    camData.farUse2D[1] = 0.0f;  // u_Use2D = 0.0
    camData.farUse2D[2] = 0.0f;
    camData.farUse2D[3] = 0.0f;

    extra.cameraUniformBuffer->UpdateData(&camData, sizeof(camData), 0);
}

// ============================================================================
// UpdateParticles — 每帧更新：发射新粒子 + dispatch compute shader
// ============================================================================

void GPUParticleSystem::UpdateParticles(float dt, const Vector3& cameraPosition,
                                        const Vector3& emitterPosition,
                                        const EmitterConfig& config) {
    if (!m_Initialized || !m_ComputePipeline) return;

    // 根据距离做 LOD
    float dist = glm::distance(cameraPosition, emitterPosition);
    if (dist > config.maxDistance) return;

    float lodFactor = 1.0f;
    if (dist > config.lodDistance) {
        lodFactor = config.lodFactor;
    }

    // === 发射新粒子（CPU 端） ===
    if (config.spawnRate > 0.0f) {
        m_SpawnAccumulator += config.spawnRate * dt * lodFactor;
    }

    uint32_t spawnCount = static_cast<uint32_t>(m_SpawnAccumulator);
    if (spawnCount > 0) {
        m_SpawnAccumulator -= static_cast<float>(spawnCount);

        // 限制不超过剩余空闲槽位
        uint32_t freeSlots = m_MaxParticles;
        if (m_AliveCount < m_MaxParticles) {
            freeSlots = m_MaxParticles - m_AliveCount;
        }
        spawnCount = std::min(spawnCount, freeSlots);
        spawnCount = std::min(spawnCount, m_MaxParticles / 4u); // 每帧最多 1/4 总量

        if (spawnCount > 0) {
            EmitterConfig adjustedConfig = config;
            adjustedConfig.position = emitterPosition;
            SpawnParticles(spawnCount, adjustedConfig);
        }
    }

    if (!m_ComputeShader) return;

    // === 更新 uniform 数据 ===
    {
        ParticleUniformData uniformData;
        uniformData.deltaTime = dt;
        uniformData.gravityX = config.gravity.x;
        uniformData.gravityY = config.gravity.y;
        uniformData.gravityZ = config.gravity.z;
        uniformData.aliveCount = m_AliveCount;
        uniformData.maxParticles = m_MaxParticles;
        uniformData.padding0 = 0.0f;
        uniformData.padding1 = 0.0f;

        m_UniformBuffer->UpdateData(&uniformData, sizeof(uniformData), 0);
    }

    // 更新间接缓冲区（CPU 端维护 alive count）
    UpdateIndirectBuffer();
}

// ============================================================================
// RenderParticles — 渲染粒子（在渲染通道内调用）
// ============================================================================

void GPUParticleSystem::RenderParticles(Graphic::ICamera* camera,
                                        Graphic::ICommandBuffer* cmdBuffer) {
    if (!m_Initialized || !cmdBuffer || !camera) return;
    if (!m_GraphicsPipeline || m_AliveCount == 0) return;

    auto& extra = GetExtra(this);
    if (!extra.gfxDescriptorSet) return;

    // === 更新摄像机 Uniform（根据 2D/3D 模式） ===
    if (m_use2D) {
        Update2DCameraUniforms(camera, cmdBuffer);
    } else {
        Update3DCameraUniforms(camera, cmdBuffer);
    }

    // === Stage 1: 计算着色器更新粒子 ===
    if (m_ComputePipeline && m_DescriptorSet) {
        cmdBuffer->SetComputePipeline(m_ComputePipeline.get());
        cmdBuffer->BindDescriptorSet(0, m_DescriptorSet.get());

        uint32_t workGroups = (m_MaxParticles + 255) / 256; // local_size_x = 256
        cmdBuffer->Dispatch(std::max(workGroups, 1u), 1, 1);
    }

    // === 内存屏障：计算 → 图形 ===
    cmdBuffer->PipelineBarrier();

    // === Stage 2: 图形渲染 ===
    cmdBuffer->SetPipelineState(m_GraphicsPipeline.get());
    cmdBuffer->BindDescriptorSet(0, extra.gfxDescriptorSet.get());

    // 每粒子 4 个顶点（四边形），gl_VertexIndex / 4 索引粒子
    // 死亡粒子由顶点着色器裁剪（gl_Position = 0）
    uint32_t vertexCount = m_MaxParticles * 4;
    cmdBuffer->Draw(vertexCount, 1, 0);
}

// ============================================================================
// SpawnParticles — 生成并上传新粒子数据
// ============================================================================

void GPUParticleSystem::SpawnParticles(uint32_t count, const EmitterConfig& config) {
    if (count == 0 || !m_Initialized) return;
    if (m_AliveCount >= m_MaxParticles) return; // 已满

    // 限制不超过剩余容量
    count = std::min(count, m_MaxParticles - m_AliveCount);
    if (count == 0) return;

    // 清空之前遗留的 CPU 数据
    if (!m_ParticleCPUData.empty()) {
        // 上次未上传的数据？合并或覆盖
        m_ParticleCPUData.clear();
    }

    m_ParticleCPUData.reserve(count);

    for (uint32_t i = 0; i < count; i++) {
        GPUParticleData p;

        // 位置（在发射器形状内随机）
        Vector3 pos = RandomPositionInShape(config, m_RNG);
        // 2D 模式下强制 z=0
        if (m_use2D) {
            pos.z = 0.0f;
        }
        // 方向/速度
        Vector3 dir = RandomDirectionInShape(config, m_RNG);
        float speed = config.speed.Random(m_RNG);

        // 生命周期
        float lifetime = config.lifetime.Random(m_RNG);

        // 外观
        float size = config.startSize.Random(m_RNG);
        float rotation = config.rotation.Random(m_RNG);
        Vector4 color = config.startColor.Random(m_RNG);

        // 填充 GPU 粒子数据结构（匹配 GLSL 布局）
        // position: xyz = 位置, w = 尺寸
        p.position = Vector4(pos.x, pos.y, pos.z, size);

        // velocity: xyz = 速度, w = 旋转角
        p.velocity = Vector4(dir.x * speed, dir.y * speed, dir.z * speed, glm::radians(rotation));

        // color: rgba
        p.color = color;

        // life: x = 年龄(0), y = 总生命周期, z = unused, w = 存活标记(>0)
        p.life = Vector4(0.0f, lifetime, 0.0f, 1.0f);

        m_ParticleCPUData.push_back(p);
    }

    // 上传到 GPU
    UploadParticleData();
}

} // namespace Prisma::Particles
