#include "terrain/TerrainSystem.h"
#include "Logger.h"
#include "Engine.h"
#include "SceneManager.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IPipeline.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/RenderDesc.h"
#include "transform/Camera.h"
#include <algorithm>
#include <cmath>

namespace Prisma::Terrain {

// ============================================================================
// Uniform 缓冲布局（必须匹配 terrain.vert/frag 中的 TerrainUniforms）
// ============================================================================

struct alignas(16) TerrainUniformData {
    PrismaMath::mat4 viewProjection;   // 0-63
    PrismaMath::vec4 cameraPos;        // 64-79: xyz=pos, w=padding
    PrismaMath::vec4 blendHeights;     // 80-95: 4层混合中心高度
    PrismaMath::vec4 blendWidths;      // 96-111
    PrismaMath::vec4 slopeFactors;     // 112-127
    PrismaMath::vec4 tiling;           // 128-143: x=tiling0-1, y=tiling2-3
    PrismaMath::vec4 heightRange;      // 144-159: x=min, y=max, z=invRange, w=unused
    PrismaMath::vec4 pbrParams;        // 160-175: x=roughness, y=metallic, z=ao, w=unused
    PrismaMath::vec4 sunDirection;     // 176-191
    PrismaMath::vec4 sunColor;         // 192-207: rgb=color*intensity, w=ambient
};

// ============================================================================
// 构造/析构
// ============================================================================

TerrainSystem::TerrainSystem()
{
    m_TerrainMesh = std::make_shared<TerrainMesh>();
    m_Collision.SetHeightMap(&m_HeightMap);
}

TerrainSystem::~TerrainSystem()
{
    Shutdown();
}

// ============================================================================
// ISubSystem 接口
// ============================================================================

int TerrainSystem::Initialize()
{
    if (m_Initialized) return 0;

    LOG_INFO("Terrain", "地形系统正在初始化...");

    // 生成测试高度图作为默认
    if (!m_HeightMap.IsValid()) {
        GenerateTestTerrain(512, 512, 0.015f, 30.0f);
    }

    // 初始化默认材质
    InitDefaultMaterial();

    // 生成初始网格
    GenerateInitialMesh();

    // 创建 GPU 资源（管线、缓冲区）
    CreateGPUResources();

    m_Initialized = true;
    LOG_INFO("Terrain", "地形系统初始化完成");
    return 0;
}

void TerrainSystem::Shutdown()
{
    if (!m_Initialized) return;

    LOG_INFO("Terrain", "地形系统正在关闭...");

    DestroyGPUResources();

    m_TerrainMesh->Clear();
    m_HeightMap = HeightMap();
    m_Material.ClearLayers();

    m_Initialized = false;
    LOG_INFO("Terrain", "地形系统已关闭");
}

void TerrainSystem::Update(Timestep ts)
{
    if (!m_Initialized || !m_Enabled) return;
    if (!m_HeightMap.IsValid()) return;

    float dt = ts.GetSeconds();
    if (dt <= 0.0f || dt > 0.1f) dt = 0.1f;

    // 更新视锥
    UpdateFrustum();

    // 更新 LOD（基于相机位置）
    UpdateLOD(m_CameraPosition);

    // 提交渲染命令
    SubmitDrawCalls();
}

// ============================================================================
// 高度图加载
// ============================================================================

bool TerrainSystem::LoadHeightMap(const std::string& filepath,
                                   uint32_t width, uint32_t height,
                                   float heightScale)
{
    bool success = m_HeightMap.LoadFromRaw(filepath, width, height, heightScale);
    if (success) {
        m_Collision.SetHeightMap(&m_HeightMap);
        m_Material.SetMinHeight(m_HeightMap.GetMinHeight());
        m_Material.SetMaxHeight(m_HeightMap.GetMaxHeight());
        ForceRegenerateMesh();
    }
    return success;
}

bool TerrainSystem::LoadHeightMapPNG(const std::string& filepath,
                                      float heightScale)
{
    bool success = m_HeightMap.LoadFromPNG(filepath, heightScale);
    if (success) {
        m_Collision.SetHeightMap(&m_HeightMap);
        m_Material.SetMinHeight(m_HeightMap.GetMinHeight());
        m_Material.SetMaxHeight(m_HeightMap.GetMaxHeight());
        ForceRegenerateMesh();
    }
    return success;
}

bool TerrainSystem::LoadHeightMapFromArray(const float* data,
                                            uint32_t width, uint32_t height)
{
    bool success = m_HeightMap.LoadFromFloatArray(data, width, height);
    if (success) {
        m_Collision.SetHeightMap(&m_HeightMap);
        m_Material.SetMinHeight(m_HeightMap.GetMinHeight());
        m_Material.SetMaxHeight(m_HeightMap.GetMaxHeight());
        ForceRegenerateMesh();
    }
    return success;
}

void TerrainSystem::GenerateTestTerrain(uint32_t width, uint32_t height,
                                         float frequency, float amplitude)
{
    m_HeightMap.GenerateTestTerrain(width, height, frequency, amplitude);
    m_Collision.SetHeightMap(&m_HeightMap);
    m_Material.SetMinHeight(m_HeightMap.GetMinHeight());
    m_Material.SetMaxHeight(m_HeightMap.GetMaxHeight());
    ForceRegenerateMesh();
}

// ============================================================================
// 网格管理
// ============================================================================

void TerrainSystem::ForceRegenerateMesh()
{
    if (!m_HeightMap.IsValid()) return;
    m_TerrainMesh->ForceGenerate(m_HeightMap, m_CameraPosition, m_Config);
    UploadLODBuffers();
}

// ============================================================================
// 内部更新
// ============================================================================

void TerrainSystem::InitDefaultMaterial()
{
    m_Material.LoadFromConfig(m_Config);
    m_Material.SetMinHeight(m_HeightMap.GetMinHeight());
    m_Material.SetMaxHeight(m_HeightMap.GetMaxHeight());
}

void TerrainSystem::GenerateInitialMesh()
{
    if (!m_HeightMap.IsValid()) return;

    m_TerrainMesh->ForceGenerate(m_HeightMap, Vector3(0.0f), m_Config);
    m_VisibleLODCount = static_cast<uint32_t>(m_TerrainMesh->GetLODCount());
}

void TerrainSystem::UpdateLOD(const Vector3& cameraPosition)
{
    if (!m_TerrainMesh) return;

    m_TerrainMesh->Generate(m_HeightMap, cameraPosition, m_Config);

    // 如果 LOD 数据更新，重新上传缓冲
    UploadLODBuffers();

    // 执行视锥剔除
    const auto& frustum = m_FrustumCulling.getFrustum();
    auto visibility = m_TerrainMesh->CullAgainstFrustum(frustum);

    // 统计可见 LOD 数量
    m_VisibleLODCount = 0;
    for (bool visible : visibility) {
        if (visible) ++m_VisibleLODCount;
    }
}

void TerrainSystem::UpdateFrustum()
{
    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) return;

    auto* device = renderSystem->GetDevice();
    if (!device) return;

    // 从渲染系统获取相机参数并更新视锥
    // 如果场景管理器中有相机，使用相机参数
    auto* sceneManager = engine.GetSceneManager();
    if (sceneManager) {
        auto* scene = sceneManager->GetCurrentScene();
        if (scene) {
            auto camera = scene->GetMainCamera();
            if (camera) {
                m_CameraPosition = camera->GetPosition();
                m_CameraForward = camera->GetForward();
                m_CameraUp = camera->GetUp();
                m_CameraFOV = camera->GetFOV();
                m_CameraAspect = camera->GetAspectRatio();
                m_NearPlane = camera->GetNearPlane();
                m_FarPlane = camera->GetFarPlane();
            }
        }
    }

    // 更新视锥体
    m_FrustumCulling.updateFrustum(
        glm::dvec3(m_CameraPosition.x, m_CameraPosition.y, m_CameraPosition.z),
        glm::dvec3(m_CameraForward.x, m_CameraForward.y, m_CameraForward.z),
        glm::dvec3(m_CameraUp.x, m_CameraUp.y, m_CameraUp.z),
        static_cast<double>(m_CameraFOV),
        static_cast<double>(m_CameraAspect),
        static_cast<double>(m_NearPlane),
        static_cast<double>(m_FarPlane)
    );
}

// ============================================================================
// GPU 资源创建
// ============================================================================

void TerrainSystem::CreateGPUResources()
{
    if (m_GPUResourcesCreated) return;

    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) {
        LOG_WARNING("Terrain", "渲染系统不可用，跳过 GPU 资源创建");
        return;
    }

    auto* device = renderSystem->GetDevice();
    if (!device) {
        LOG_WARNING("Terrain", "渲染设备不可用，跳过 GPU 资源创建");
        return;
    }

    auto* factory = device->GetResourceFactory();
    if (!factory) {
        LOG_WARNING("Terrain", "资源工厂不可用，跳过 GPU 资源创建");
        return;
    }

    auto* resourceManager = engine.GetRenderResourceManager();
    if (!resourceManager) {
        LOG_WARNING("Terrain", "资源管理器不可用，跳过 GPU 资源创建");
        return;
    }

    // 1. 加载着色器
    m_TerrainVertShader = resourceManager->LoadShaderSync(
        "assets/shaders/terrain.vert.spv", "main");
    if (!m_TerrainVertShader) {
        LOG_ERROR("Terrain", "无法加载地形顶点着色器: assets/shaders/terrain.vert.spv");
        return;
    }

    m_TerrainFragShader = resourceManager->LoadShaderSync(
        "assets/shaders/terrain.frag.spv", "main");
    if (!m_TerrainFragShader) {
        LOG_ERROR("Terrain", "无法加载地形片段着色器: assets/shaders/terrain.frag.spv");
        return;
    }

    // ---- 2. 创建管线状态对象 ----
    auto pso = factory->CreatePipelineStateImpl();
    if (!pso) {
        LOG_ERROR("Terrain", "创建 PSO 失败");
        return;
    }

    pso->SetShader(Graphic::ShaderType::Vertex, m_TerrainVertShader);
    pso->SetShader(Graphic::ShaderType::Pixel, m_TerrainFragShader);
    pso->SetPrimitiveTopology(Graphic::PrimitiveTopology::TriangleList);

    // 顶点输入布局: TerrainVertex = { vec3 pos, vec3 normal, vec2 uv }
    std::vector<Graphic::VertexInputAttribute> attributes = {
        { "POSITION", 0, Graphic::TextureFormat::RGB32_Float, 0, 0  },
        { "NORMAL",   0, Graphic::TextureFormat::RGB32_Float, 0, 12 },
        { "TEXCOORD", 0, Graphic::TextureFormat::RG32_Float,  0, 24 },
    };
    pso->SetInputLayout(attributes);

    // 光栅化状态: 背面剔除
    Graphic::RasterizerState rs;
    rs.cullMode = Graphic::CullMode::Back;
    rs.frontCounterClockwise = true;
    pso->SetRasterizerState(rs);

    // 深度模板: 开启深度测试和写入
    Graphic::DepthStencilState ds{};
    ds.depthEnable = true;
    ds.depthWriteEnable = true;
    ds.depthFunc = Graphic::ComparisonFunc::Less;
    pso->SetDepthStencilState(ds);

    // 混合状态: 不透明
    Graphic::BlendState blend{};
    blend.blendEnable = false;
    pso->SetBlendState(blend);

    if (!pso->Create(device)) {
        LOG_ERROR("Terrain", "创建地形管线失败: {}", pso->GetErrors());
        return;
    }

    m_TerrainPipeline = std::shared_ptr<Graphic::IPipelineState>(std::move(pso));
    LOG_INFO("Terrain", "地形管线创建成功");

    // ---- 3. 创建 Uniform 缓冲 ----
    Graphic::BufferDesc uboDesc;
    uboDesc.type = Graphic::BufferType::Constant;
    uboDesc.size = sizeof(TerrainUniformData);
    uboDesc.usage = Graphic::BufferUsage::Dynamic;
    uboDesc.name = "TerrainUniformBuffer";
    m_UniformBuffer = factory->CreateBufferImpl(uboDesc);
    if (!m_UniformBuffer) {
        LOG_ERROR("Terrain", "创建 Uniform 缓冲失败");
        DestroyGPUResources();
        return;
    }

    // ---- 4. 创建描述符集布局和描述符集 ----
    {
        std::vector<Graphic::ShaderResource> resources;

        // Binding 0: TerrainUniforms (UBO)
        Graphic::ShaderResource uboRes;
        uboRes.Name = "TerrainUniforms";
        uboRes.ResourceType = Graphic::ShaderResource::Type::UniformBuffer;
        uboRes.Set = 0;
        uboRes.Binding = 0;
        uboRes.Size = static_cast<uint32_t>(sizeof(TerrainUniformData));
        resources.push_back(uboRes);

        // Binding 1-4: Splat textures (Sampler2D)
        for (uint32_t i = 0; i < 4; ++i) {
            Graphic::ShaderResource texRes;
            texRes.Name = "u_SplatTexture" + std::to_string(i);
            texRes.ResourceType = Graphic::ShaderResource::Type::Sampler2D;
            texRes.Set = 0;
            texRes.Binding = 1 + i;
            resources.push_back(texRes);
        }

        m_DescriptorSetLayout = factory->CreateDescriptorSetLayout(resources);
        if (!m_DescriptorSetLayout) {
            LOG_ERROR("Terrain", "创建描述符集布局失败");
            DestroyGPUResources();
            return;
        }

        m_DescriptorSet = factory->CreateDescriptorSet(m_DescriptorSetLayout.get());
        if (!m_DescriptorSet) {
            LOG_ERROR("Terrain", "创建描述符集失败");
            DestroyGPUResources();
            return;
        }

        // 绑定 Uniform 缓冲到描述符集
        m_DescriptorSet->BindBuffer(
            0, m_UniformBuffer.get(), 0,
            sizeof(TerrainUniformData),
            Graphic::DescriptorType::UniformBuffer
        );

        // 纹理绑定额外处理: 实际纹理对象由外部提供
        // 此处先绑定空描述符（Update 后布局固定）
        m_DescriptorSet->Update();
    }

    // ---- 5. 上传初始 LOD 缓冲 ----
    UploadLODBuffers();

    m_GPUResourcesCreated = true;
    LOG_INFO("Terrain", "GPU 资源创建完成 ({} LOD 级)", m_UploadedLODCount);
}

void TerrainSystem::DestroyGPUResources()
{
    m_DescriptorSet.reset();
    m_DescriptorSetLayout.reset();
    m_UniformBuffer.reset();
    for (auto& vb : m_VertexBuffers) vb.reset();
    for (auto& ib : m_IndexBuffers) ib.reset();
    m_TerrainPipeline.reset();
    m_TerrainFragShader.reset();
    m_TerrainVertShader.reset();
    m_UploadedLODCount = 0;
    m_GPUResourcesCreated = false;
}

void TerrainSystem::UploadLODBuffers()
{
    if (!m_TerrainMesh || !m_TerrainMesh->IsValid()) return;
    if (!m_GPUResourcesCreated) return;

    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) return;
    auto* device = renderSystem->GetDevice();
    if (!device) return;
    auto* factory = device->GetResourceFactory();
    if (!factory) return;

    const auto& levels = m_TerrainMesh->GetLODLevels();
    uint32_t lodCount = std::min(static_cast<uint32_t>(levels.size()),
                                 static_cast<uint32_t>(k_MaxLODLevels));

    for (uint32_t i = 0; i < lodCount; ++i) {
        const auto& level = levels[i];
        if (level.IsEmpty()) continue;

        bool needsUpdate = false;

        // 如果缓冲未创建或顶点数变化，重新创建
        if (!m_VertexBuffers[i] ||
            m_VertexBuffers[i]->GetElementCount() != level.GetVertexCount()) {
            needsUpdate = true;
        }
        if (!m_IndexBuffers[i] ||
            m_IndexBuffers[i]->GetElementCount() != level.GetIndexCount()) {
            needsUpdate = true;
        }

        if (!needsUpdate) continue;

        // 创建/更新顶点缓冲
        size_t vbSize = level.vertices.size() * sizeof(TerrainVertex);
        Graphic::BufferDesc vbDesc;
        vbDesc.type = Graphic::BufferType::Vertex;
        vbDesc.size = vbSize;
        vbDesc.usage = Graphic::BufferUsage::Immutable;
        vbDesc.initialData = level.vertices.data();
        vbDesc.stride = sizeof(TerrainVertex);
        vbDesc.name = "TerrainVB_LOD" + std::to_string(i);
        m_VertexBuffers[i] = factory->CreateBufferImpl(vbDesc);

        if (!m_VertexBuffers[i]) {
            LOG_ERROR("Terrain", "创建 LOD{} 顶点缓冲失败 ({} 字节)", i, vbSize);
            continue;
        }

        // 创建/更新索引缓冲
        size_t ibSize = level.indices.size() * sizeof(uint32_t);
        Graphic::BufferDesc ibDesc;
        ibDesc.type = Graphic::BufferType::Index;
        ibDesc.size = ibSize;
        ibDesc.usage = Graphic::BufferUsage::Immutable;
        ibDesc.initialData = level.indices.data();
        ibDesc.name = "TerrainIB_LOD" + std::to_string(i);
        m_IndexBuffers[i] = factory->CreateBufferImpl(ibDesc);

        if (!m_IndexBuffers[i]) {
            LOG_ERROR("Terrain", "创建 LOD{} 索引缓冲失败 ({} 字节)", i, ibSize);
            m_VertexBuffers[i].reset();
            continue;
        }

        LOG_DEBUG("Terrain", "LOD{} GPU 缓冲已上传: {} 顶点, {} 索引",
                  i, level.GetVertexCount(), level.GetIndexCount());
    }

    m_UploadedLODCount = lodCount;
}

void TerrainSystem::UpdateUniformBuffer()
{
    if (!m_UniformBuffer) return;

    TerrainUniformData data{};

    // 相机: 视图投影矩阵
    auto& engine = Engine::Get();
    auto* sceneManager = engine.GetSceneManager();
    PrismaMath::mat4 viewMatrix(1.0f);
    PrismaMath::mat4 projMatrix(1.0f);

    if (sceneManager) {
        auto* scene = sceneManager->GetCurrentScene();
        if (scene) {
            auto camera = scene->GetMainCamera();
            if (camera) {
                auto glmView = camera->GetViewMatrix();
                auto glmProj = camera->GetProjectionMatrix();
                memcpy(&viewMatrix, &glmView, sizeof(PrismaMath::mat4));
                memcpy(&projMatrix, &glmProj, sizeof(PrismaMath::mat4));
            }
        }
    }

    // 计算 viewProjection
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            data.viewProjection[c][r] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                data.viewProjection[c][r] += projMatrix[c][k] * viewMatrix[k][r];
            }
        }
    }

    data.cameraPos = PrismaMath::vec4(
        m_CameraPosition.x, m_CameraPosition.y, m_CameraPosition.z, 0.0f
    );

    // 默认值（从材质配置填充）
    data.blendHeights = PrismaMath::vec4(10.0f, 50.0f, 5.0f, 100.0f);
    data.blendWidths  = PrismaMath::vec4(15.0f, 20.0f, 10.0f, 15.0f);
    data.slopeFactors = PrismaMath::vec4(0.0f, 0.7f, 0.0f, 0.3f);
    data.tiling       = PrismaMath::vec4(32.0f, 64.0f, 0.0f, 0.0f);

    // 从材质层填充（最多 4 层）
    for (size_t i = 0; i < std::min(m_Material.GetLayerCount(), size_t(4)); ++i) {
        const auto& layer = m_Material.GetLayer(static_cast<uint32_t>(i));
        float* heights = &data.blendHeights.x;
        float* widths  = &data.blendWidths.x;
        float* slopes  = &data.slopeFactors.x;
        heights[i] = layer.blendHeight;
        widths[i]  = layer.blendWidth;
        slopes[i]  = layer.slopeFactor;
        if (i < 2) {
            data.tiling.x = layer.tiling.x;
        } else {
            data.tiling.y = layer.tiling.x;
        }
    }

    // 高度范围
    float minH = m_HeightMap.GetMinHeight();
    float maxH = m_HeightMap.GetMaxHeight();
    float range = maxH - minH;
    data.heightRange = PrismaMath::vec4(minH, maxH,
        range > 0.001f ? 1.0f / range : 1.0f, 0.0f);

    // PBR 参数
    data.pbrParams = PrismaMath::vec4(0.7f, 0.0f, 1.0f, 0.0f);

    // 默认方向光
    data.sunDirection = PrismaMath::vec4(0.5f, 0.8f, 0.3f, 0.0f);
    data.sunColor = PrismaMath::vec4(1.0f, 0.95f, 0.9f, 0.05f);

    m_UniformBuffer->UpdateData(&data, sizeof(data), 0);
}

// ============================================================================
// 渲染
// ============================================================================

void TerrainSystem::SubmitDrawCalls()
{
    if (!m_TerrainMesh || !m_TerrainMesh->IsValid()) return;
    if (!m_GPUResourcesCreated) return;

    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) return;

    auto* device = renderSystem->GetDevice();
    if (!device) return;

    // 获取当前命令缓冲 (需要在渲染通道中)
    auto* cmd = device->GetCurrentCommandBuffer();
    if (!cmd) return;

    // 检查管线是否有效
    if (!m_TerrainPipeline || !m_TerrainPipeline->IsValid()) return;

    // 更新 Uniform 缓冲
    UpdateUniformBuffer();

    // 更新描述符集（如果需要）
    if (m_DescriptorSet) {
        m_DescriptorSet->Update();
    }

    // 绑定管线
    cmd->SetPipelineState(m_TerrainPipeline.get());

    // 设置视口和裁剪矩形
    Graphic::Viewport vp;
    vp.x = 0.0f;
    vp.y = 0.0f;
    vp.width = static_cast<float>(m_CameraAspect > 0.0f ? 1600.0f : 1600.0f);
    vp.height = static_cast<float>(vp.width / m_CameraAspect);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    cmd->SetViewport(vp);

    Graphic::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = static_cast<int>(vp.width);
    scissor.height = static_cast<int>(vp.height);
    cmd->SetScissorRect(scissor);

    // 绑定描述符集
    if (m_DescriptorSet) {
        cmd->BindDescriptorSet(0, m_DescriptorSet.get());
    }

    // 获取可见性
    const auto& frustum = m_FrustumCulling.getFrustum();
    auto visibility = m_TerrainMesh->CullAgainstFrustum(frustum);

    // 提交每个可见 LOD 等级的绘制调用
    const auto& levels = m_TerrainMesh->GetLODLevels();
    for (size_t i = 0; i < levels.size(); ++i) {
        if (i < visibility.size() && !visibility[i]) continue;
        if (i >= m_UploadedLODCount) continue;

        const auto& level = levels[i];
        if (level.IsEmpty()) continue;

        auto* vb = m_VertexBuffers[i].get();
        auto* ib = m_IndexBuffers[i].get();
        if (!vb || !ib) continue;

        // 绑定顶点和索引缓冲
        cmd->SetVertexBuffer(vb, 0);
        cmd->SetIndexBuffer(ib, true); // 32-bit 索引

        // 发出绘制调用
        uint32_t indexCount = static_cast<uint32_t>(level.indices.size());
        cmd->DrawIndexed(indexCount);
    }
}

// ============================================================================
// 统计
// ============================================================================

size_t TerrainSystem::GetTotalTriangleCount() const
{
    if (!m_TerrainMesh) return 0;

    size_t total = 0;
    for (const auto& level : m_TerrainMesh->GetLODLevels()) {
        total += level.GetTriangleCount();
    }
    return total;
}

} // namespace Prisma::Terrain
