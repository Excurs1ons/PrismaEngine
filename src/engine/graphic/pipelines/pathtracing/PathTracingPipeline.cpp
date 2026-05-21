#include "PathTracingPipeline.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/ISampler.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/Renderer.h"
#include "graphic/RenderDesc.h"
#include "graphic/OrthographicCamera.h"
#include "graphic/interfaces/ShaderReflection.h"
#include "graphic/interfaces/IMesh.h"
#include "graphic/MeshRenderer.h"
#include "scene/Scene.h"
#include <cstring>
#include "transform/Transform.h"
#include "Logger.h"
#include "utils/ImageUtils.h"
#include "graphic/ICamera.h"
#include "graphic/interfaces/IResourceManager.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "graphic/adapters/vulkan/VulkanResources.h"
#include "graphic/adapters/vulkan/VulkanCommandBuffer.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <functional>
#include <fstream>
#include <sstream>

// Gizmo push constants — 必须与 gizmo shader 布局一致
// 注意：位于全局命名空间，不可使用 PrismaMath 别名
namespace {
struct alignas(16) GizmoPushConstants {
    glm::mat4 mvp;
    glm::vec4 color;
};
}

namespace Prisma::Graphic {

PathTracingPipeline::PathTracingPipeline() = default;
PathTracingPipeline::~PathTracingPipeline() { Shutdown(); }

void PathTracingPipeline::SetComputeShaderSPIRV(const void* data, size_t size) {
    m_computeSPIRV.resize(size);
    std::memcpy(m_computeSPIRV.data(), data, size);
}

void PathTracingPipeline::SetPresentShadersSPIRV(const void* vertData, size_t vertSize,
                                                   const void* fragData, size_t fragSize) {
    m_presentVertSPIRV.resize(vertSize);
    m_presentFragSPIRV.resize(fragSize);
    std::memcpy(m_presentVertSPIRV.data(), vertData, vertSize);
    std::memcpy(m_presentFragSPIRV.data(), fragData, fragSize);
}

int PathTracingPipeline::Initialize(IRenderDevice* device) {
    if (!device) {
        LOG_ERROR("PathTracingPipeline", "设备为空");
        return -1;
    }

    // 如果未显式设置着色器，从默认路径内部加载
    if (!m_computeShader && m_computeSPIRV.empty()) {
        LoadDefaultShaders();
    }

    bool hasCompute = m_computeShader || !m_computeSPIRV.empty();
    bool hasPresent = (m_presentVertShader && m_presentFragShader) ||
                      (!m_presentVertSPIRV.empty() && !m_presentFragSPIRV.empty());
    if (!hasCompute || !hasPresent) {
        LOG_ERROR("PathTracingPipeline", "着色器数据未设置（需要 SPIR-V 或 IShader）");
        return -1;
    }

    m_device = device;

    // m_width/m_height 在成员初始化中已为 0，首帧 Execute 会通过
    // ResizeResources 自动调整到窗口实际尺寸

    if (!CreateResources()) {
        LOG_ERROR("PathTracingPipeline", "创建资源失败");
        DestroyResources();
        return -1;
    }

    // 自动从当前场景构建路径追踪数据（如未通过 SetSceneData/SetTriangleData 显式设置）
    // 使用 m_scene 指针防止重复构建（OnSceneLoaded 可能随后以相同场景调用）
    if (m_cachedSceneData.objectCount == 0 && m_cachedTriangleData.triangleCount == 0) {
        auto* sceneManager = Engine::Get().GetSceneManager();
        if (sceneManager) {
            auto* scene = sceneManager->GetCurrentScene();
            if (scene && scene != m_scene) {
                BuildFromScene(scene);
            }
        }
    }

    // 将缓存数据上传到 GPU（首次初始化或重建后）
    if (m_cachedSceneData.objectCount > 0 && m_sceneSSBO) {
        m_sceneSSBO->UpdateData(&m_cachedSceneData, sizeof(PathTracingSceneData), 0);
    }
    if (m_cachedTriangleData.triangleCount > 0 && m_triangleBuffer) {
        m_triangleBuffer->UpdateData(&m_cachedTriangleData, sizeof(PathTracingTriangleData), 0);
    }
    if (m_bvhNodeCount > 0 && m_bvhBuffer) {
        m_bvhBuffer->UpdateData(m_bvhNodes.data(), m_bvhNodeCount * sizeof(BVHNode), 0);
    }
    if (!m_triToObject.empty() && m_triToObjectBuffer) {
        m_triToObjectBuffer->UpdateData(m_triToObject.data(), (uint32_t)(m_triToObject.size() * sizeof(int)), 0);
    }

    m_initialized = true;
    LOG_INFO("PathTracingPipeline", "路径追踪管线初始化完成 ({}x{})",
             m_width ? m_width : 1280, m_height ? m_height : 720);
    return 0;
}

void PathTracingPipeline::OnSceneLoaded(::Prisma::Scene* scene) {
    if (m_scene == scene) {
        LOG_DEBUG("PathTracingPipeline", "OnSceneLoaded 跳过（场景已构建）");
        return;
    }
    BuildFromScene(scene);

    // 标记下次切换/使用 RT 时需要重建
    m_sceneChangedSinceLastRTBuild = true;

    if (m_mode == PathTraceMode::BVH) {
        // BVH 模式：预变换顶点到世界空间 + 构建 BVH 加速结构
        for (int i = 0; i < m_cachedSceneData.objectCount; i++) {
            Node node(m_cachedNodeHandles[i]);
            if (!node.IsValid()) continue;
            Matrix4x4 worldMat = scene->GetWorldTransform(node);
            std::memcpy(m_cachedSceneData.objects[i].worldMatrix, &worldMat, sizeof(float) * 16);
        }
        for (uint32_t oi = 0; oi < (uint32_t)m_cachedSceneData.objectCount; oi++) {
            auto& obj = m_cachedSceneData.objects[oi];
            int type = (int)obj.p0[3];
            if (type != 4) continue;
            int firstTri = (int)obj.p1[0];
            int triCnt = (int)obj.p1[1];
            glm::mat4 worldMat;
            std::memcpy(&worldMat, obj.worldMatrix, sizeof(float) * 16);
            glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(worldMat)));
            for (int t = 0; t < triCnt; t++) {
                auto& tri = m_cachedTriangleData.triangles[firstTri + t];
                glm::vec4 v0 = worldMat * glm::vec4(tri.vertices[0].pos[0], tri.vertices[0].pos[1], tri.vertices[0].pos[2], 1.0f);
                glm::vec4 v1 = worldMat * glm::vec4(tri.vertices[1].pos[0], tri.vertices[1].pos[1], tri.vertices[1].pos[2], 1.0f);
                glm::vec4 v2 = worldMat * glm::vec4(tri.vertices[2].pos[0], tri.vertices[2].pos[1], tri.vertices[2].pos[2], 1.0f);
                tri.vertices[0].pos[0] = v0.x; tri.vertices[0].pos[1] = v0.y; tri.vertices[0].pos[2] = v0.z;
                tri.vertices[1].pos[0] = v1.x; tri.vertices[1].pos[1] = v1.y; tri.vertices[1].pos[2] = v1.z;
                tri.vertices[2].pos[0] = v2.x; tri.vertices[2].pos[1] = v2.y; tri.vertices[2].pos[2] = v2.z;
                // 法线使用逆转置矩阵变换（均匀缩放+旋转时 normalMat = mat3(worldMat)）
                glm::vec3 n0 = normalMat * glm::vec3(tri.vertices[0].nrm[0], tri.vertices[0].nrm[1], tri.vertices[0].nrm[2]);
                glm::vec3 n1 = normalMat * glm::vec3(tri.vertices[1].nrm[0], tri.vertices[1].nrm[1], tri.vertices[1].nrm[2]);
                glm::vec3 n2 = normalMat * glm::vec3(tri.vertices[2].nrm[0], tri.vertices[2].nrm[1], tri.vertices[2].nrm[2]);
                tri.vertices[0].nrm[0] = n0.x; tri.vertices[0].nrm[1] = n0.y; tri.vertices[0].nrm[2] = n0.z;
                tri.vertices[1].nrm[0] = n1.x; tri.vertices[1].nrm[1] = n1.y; tri.vertices[1].nrm[2] = n1.z;
                tri.vertices[2].nrm[0] = n2.x; tri.vertices[2].nrm[1] = n2.y; tri.vertices[2].nrm[2] = n2.z;
            }
        }
        BuildBVH();
    }
}

bool PathTracingPipeline::CreateResources() {
    auto* factory = m_device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("PathTracingPipeline", "无法获取资源工厂");
        return false;
    }

    // 1. 创建存储纹理（计算着色器写入+片段着色器采样）
    TextureDesc texDesc{};
    texDesc.type = TextureType::Texture2D;
    texDesc.format = TextureFormat::RGBA16_Float;
    // 半分辨率渲染：路径追踪计算在 (width/2) × (height/2) 上执行，present 时硬件双线性上采样
    uint32_t createW = m_width ? m_width : 1280;
    uint32_t createH = m_height ? m_height : 720;
    texDesc.width  = createW / 2;
    texDesc.height = createH / 2;
    texDesc.depth = 1;
    texDesc.mipLevels = 1;
    texDesc.arraySize = 1;
    texDesc.allowShaderResource = true;
    texDesc.allowUnorderedAccess = true;
    m_storageTexture = factory->CreateTextureImpl(texDesc);
    if (!m_storageTexture) {
        LOG_ERROR("PathTracingPipeline", "创建存储纹理失败");
        return false;
    }

    // 2. 创建 Camera UBO
    BufferDesc uboDesc{};
    uboDesc.type = BufferType::Constant;
    uboDesc.size = sizeof(PathTracingCameraUBO);
    uboDesc.usage = BufferUsage::Dynamic;
    m_cameraUBO = factory->CreateBufferImpl(uboDesc);
    if (!m_cameraUBO) {
        LOG_ERROR("PathTracingPipeline", "创建 Camera UBO 失败");
        return false;
    }

    // 3. 创建 Scene SSBO（使用 Default 而非 Dynamic，确保 GPU 正确读取）
    BufferDesc ssboDesc{};
    ssboDesc.type = BufferType::Structured;
    ssboDesc.size = sizeof(PathTracingSceneData);
    ssboDesc.usage = BufferUsage::Default;
    m_sceneSSBO = factory->CreateBufferImpl(ssboDesc);
    if (!m_sceneSSBO) {
        LOG_ERROR("PathTracingPipeline", "创建 Scene SSBO 失败");
        return false;
    }
    // 恢复场景数据（延迟初始化时 SSBO 被重建）
    if (m_cachedSceneData.objectCount > 0) {
        m_sceneSSBO->UpdateData(&m_cachedSceneData, sizeof(PathTracingSceneData), 0);
    }

    // 3b. 创建 Triangle SSBO
    BufferDesc triSSBODesc{};
    triSSBODesc.type = BufferType::Structured;
    triSSBODesc.size = sizeof(PathTracingTriangleData);
    triSSBODesc.usage = BufferUsage::Default;
    m_triangleBuffer = factory->CreateBufferImpl(triSSBODesc);
    if (!m_triangleBuffer) {
        LOG_ERROR("PathTracingPipeline", "创建 Triangle SSBO 失败");
        return false;
    }
    if (m_cachedTriangleData.triangleCount > 0) {
        m_triangleBuffer->UpdateData(&m_cachedTriangleData, sizeof(PathTracingTriangleData), 0);
    }

    // 3c. 创建 BVH Node SSBO（始终创建，flat 模式作为占位保持布局一致性）
    {
        BufferDesc bvhSSBODesc{};
        bvhSSBODesc.type = BufferType::Structured;
        bvhSSBODesc.size = MAX_BVH_NODES * sizeof(BVHNode);
        bvhSSBODesc.usage = BufferUsage::Default;
        m_bvhBuffer = factory->CreateBufferImpl(bvhSSBODesc);
        if (!m_bvhBuffer) {
            LOG_ERROR("PathTracingPipeline", "创建 BVH SSBO 失败");
            return false;
        }
        if (m_bvhNodeCount > 0) {
            m_bvhBuffer->UpdateData(m_bvhNodes.data(), m_bvhNodeCount * sizeof(BVHNode), 0);
        }
    }

    // 3d. 创建 triToObject SSBO（始终创建，flat 模式作为占位）
    {
        BufferDesc ttoSSBODesc{};
        ttoSSBODesc.type = BufferType::Structured;
        ttoSSBODesc.size = PathTracingTriangleData::MAX_TRIANGLES * sizeof(int);
        ttoSSBODesc.usage = BufferUsage::Default;
        m_triToObjectBuffer = factory->CreateBufferImpl(ttoSSBODesc);
        if (!m_triToObjectBuffer) {
            LOG_ERROR("PathTracingPipeline", "创建 triToObject SSBO 失败");
            return false;
        }
        if (!m_triToObject.empty()) {
            m_triToObjectBuffer->UpdateData(m_triToObject.data(), (uint32_t)(m_triToObject.size() * sizeof(int)), 0);
        }
    }

    // 4. 创建计算着色器 & 管线
    m_computePipeline = factory->CreateComputePipelineImpl();
    if (!m_computePipeline) {
        LOG_ERROR("PathTracingPipeline", "创建计算管线对象失败");
        return false;
    }

    if (m_computeShader) {
        m_computePipeline->SetShader(m_computeShader);
    } else {
        ShaderReflection reflection;
        reflection.Resources = {
            {"outputImage", ShaderResource::Type::Image2D, 0, 0, 1, 0},
            {"accumImage",  ShaderResource::Type::Image2D, 0, 1, 1, 0},
            {"cameraUBO",   ShaderResource::Type::UniformBuffer, 0, 2, 1, sizeof(PathTracingCameraUBO)},
            {"sceneSSBO",   ShaderResource::Type::StorageBuffer, 0, 3, 1, sizeof(PathTracingSceneData)},
            {"triangleBuf", ShaderResource::Type::StorageBuffer, 0, 4, 1, sizeof(PathTracingTriangleData)},
            {"bvhNodes",    ShaderResource::Type::StorageBuffer, 0, 5, 1, MAX_BVH_NODES * sizeof(BVHNode)},
            {"triToObject", ShaderResource::Type::StorageBuffer, 0, 6, 1, PathTracingTriangleData::MAX_TRIANGLES * sizeof(int)},
        };
        ShaderDesc shaderDesc{};
        shaderDesc.type = ShaderType::Compute;
        shaderDesc.entryPoint = "main";
        shaderDesc.language = ShaderLanguage::SPIRV;
        shaderDesc.filename = "pathtrace.comp";
        auto computeShader = factory->CreateShaderImpl(shaderDesc, m_computeSPIRV, reflection);
        if (!computeShader) {
            LOG_ERROR("PathTracingPipeline", "创建计算着色器失败");
            return false;
        }
        m_computePipeline->SetShader(std::move(computeShader));
    }
    if (!m_computePipeline->Create(m_device)) {
        LOG_ERROR("PathTracingPipeline", "编译计算管线失败");
        return false;
    }

    // 6. 创建描述符集
    const auto& layouts = m_computePipeline->GetDescriptorSetLayouts();
    if (layouts.empty()) {
        LOG_ERROR("PathTracingPipeline", "计算管线没有描述符集布局");
        return false;
    }
    m_descriptorSet = factory->CreateDescriptorSet(layouts[0].get());
    if (!m_descriptorSet) {
        LOG_ERROR("PathTracingPipeline", "创建描述符集失败");
        return false;
    }

    // 7. 绑定资源到描述符集
    m_descriptorSet->BindStorageImage(0, m_storageTexture.get());
    m_descriptorSet->BindStorageImage(1, m_storageTexture.get());
    m_descriptorSet->BindBuffer(2, m_cameraUBO.get(), 0, sizeof(PathTracingCameraUBO),
                                DescriptorType::UniformBuffer);
    m_descriptorSet->BindBuffer(3, m_sceneSSBO.get(), 0, sizeof(PathTracingSceneData),
                                DescriptorType::StorageBuffer);
    m_descriptorSet->BindBuffer(4, m_triangleBuffer.get(), 0, sizeof(PathTracingTriangleData),
                                DescriptorType::StorageBuffer);
    m_descriptorSet->BindBuffer(5, m_bvhBuffer.get(), 0, MAX_BVH_NODES * sizeof(BVHNode),
                                DescriptorType::StorageBuffer);
    m_descriptorSet->BindBuffer(6, m_triToObjectBuffer.get(), 0, PathTracingTriangleData::MAX_TRIANGLES * sizeof(int),
                                DescriptorType::StorageBuffer);
    m_descriptorSet->Update();

    // 8. 创建 Present 着色器（仅在未预创建时从 SPIR-V 创建）
    if (!m_presentVertShader || !m_presentFragShader) {
        ShaderReflection presentReflection;
        presentReflection.Resources = {
            {"presentTexture", ShaderResource::Type::Sampler2D, 0, 0, 1, 0},
        };
        ShaderDesc vertDesc{};
        vertDesc.type = ShaderType::Vertex;
        vertDesc.entryPoint = "main";
        vertDesc.language = ShaderLanguage::SPIRV;
        vertDesc.filename = "fullscreen.vert";
        m_presentVertShader = factory->CreateShaderImpl(vertDesc, m_presentVertSPIRV, presentReflection);
        if (!m_presentVertShader) {
            LOG_ERROR("PathTracingPipeline", "创建 Present 顶点着色器失败");
            return false;
        }
        ShaderDesc fragDesc{};
        fragDesc.type = ShaderType::Pixel;
        fragDesc.entryPoint = "main";
        fragDesc.language = ShaderLanguage::SPIRV;
        fragDesc.filename = "present.frag";
        m_presentFragShader = factory->CreateShaderImpl(fragDesc, m_presentFragSPIRV, presentReflection);
        if (!m_presentFragShader) {
            LOG_ERROR("PathTracingPipeline", "创建 Present 片段着色器失败");
            return false;
        }
    }

    // 9. 创建 Present 管线状态对象（无头模式跳过）
    if (!m_device->IsHeadless()) {
        auto pso = factory->CreatePipelineStateImpl();
        pso->SetShader(ShaderType::Vertex, m_presentVertShader);
        pso->SetShader(ShaderType::Pixel, m_presentFragShader);
        pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
        RasterizerState rs{};
        rs.cullMode = CullMode::None;
        pso->SetRasterizerState(rs);
        if (!pso->Create(m_device)) {
            LOG_ERROR("PathTracingPipeline", "创建 Present PSO 失败");
            return false;
        }
        m_presentPSO = std::shared_ptr<IPipelineState>(std::move(pso));

        // 10. 创建 Present 描述符集布局和描述符集
        std::vector<ShaderResource> presentResources;
        presentResources.push_back({"presentTexture", ShaderResource::Type::Sampler2D, 0, 0, 1, 0});
        m_presentDSLayout = factory->CreateDescriptorSetLayout(presentResources);
        if (!m_presentDSLayout) {
            LOG_ERROR("PathTracingPipeline", "创建 Present 描述符集布局失败");
            return false;
        }
        m_presentDescriptorSet = factory->CreateDescriptorSet(m_presentDSLayout.get());
        if (!m_presentDescriptorSet) {
            LOG_ERROR("PathTracingPipeline", "创建 Present 描述符集失败");
            return false;
        }
        SamplerDesc samplerDesc{};
        samplerDesc.filter = TextureFilter::Linear;
        samplerDesc.addressU = TextureAddressMode::Clamp;
        samplerDesc.addressV = TextureAddressMode::Clamp;
        samplerDesc.addressW = TextureAddressMode::Clamp;
        m_presentSampler = factory->CreateSamplerImpl(samplerDesc);
        m_presentDescriptorSet->BindTexture(0, m_storageTexture.get(), m_presentSampler.get());
        m_presentDescriptorSet->Update();
    } else {
        LOG_INFO("PathTracingPipeline", "无头模式：跳过 Present 资源创建");
    }

    // 11. 初始化 Overlay 资源（gizmo PSO）
    if (!m_device->IsHeadless()) {
        InitOverlayResources();
    }

    LOG_INFO("PathTracingPipeline", "路径追踪管线资源创建完成 ({}x{})",
             m_width ? m_width : 1280, m_height ? m_height : 720);
    return true;
}

bool PathTracingPipeline::ResizeResources() {
    auto* factory = m_device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("PathTracingPipeline", "无法获取资源工厂（ResizeResources）");
        return false;
    }

    // 1. 重建存储纹理（新的宽高）
    m_storageTexture.reset();
    TextureDesc texDesc{};
    texDesc.type        = TextureType::Texture2D;
    texDesc.format      = TextureFormat::RGBA16_Float;
    texDesc.width       = m_width / 2;
    texDesc.height      = m_height / 2;
    texDesc.depth       = 1;
    texDesc.mipLevels   = 1;
    texDesc.arraySize   = 1;
    texDesc.allowShaderResource  = true;
    texDesc.allowUnorderedAccess = true;
    m_storageTexture = factory->CreateTextureImpl(texDesc);
    if (!m_storageTexture) {
        LOG_ERROR("PathTracingPipeline", "重建存储纹理失败");
        return false;
    }

    // 2. 重建计算描述符集（绑定新纹理 + 已有 UBO/SSBO）
    m_descriptorSet.reset();
    const auto& layouts = m_computePipeline->GetDescriptorSetLayouts();
    if (layouts.empty()) {
        LOG_ERROR("PathTracingPipeline", "计算管线没有描述符集布局（ResizeResources）");
        return false;
    }
    m_descriptorSet = factory->CreateDescriptorSet(layouts[0].get());
    if (!m_descriptorSet) {
        LOG_ERROR("PathTracingPipeline", "重建计算描述符集失败");
        return false;
    }
    m_descriptorSet->BindStorageImage(0, m_storageTexture.get());
    m_descriptorSet->BindStorageImage(1, m_storageTexture.get());
    m_descriptorSet->BindBuffer(2, m_cameraUBO.get(), 0, sizeof(PathTracingCameraUBO),
                                DescriptorType::UniformBuffer);
    m_descriptorSet->BindBuffer(3, m_sceneSSBO.get(), 0, sizeof(PathTracingSceneData),
                                DescriptorType::StorageBuffer);
    m_descriptorSet->BindBuffer(4, m_triangleBuffer.get(), 0, sizeof(PathTracingTriangleData),
                                DescriptorType::StorageBuffer);
    m_descriptorSet->BindBuffer(5, m_bvhBuffer.get(), 0, MAX_BVH_NODES * sizeof(BVHNode),
                                DescriptorType::StorageBuffer);
    m_descriptorSet->BindBuffer(6, m_triToObjectBuffer.get(), 0, PathTracingTriangleData::MAX_TRIANGLES * sizeof(int),
                                DescriptorType::StorageBuffer);
    m_descriptorSet->Update();

    // 3. 重建 Present 描述符集（绑定新纹理）
    if (!m_device->IsHeadless()) {
        m_presentDescriptorSet.reset();
        m_presentDescriptorSet = factory->CreateDescriptorSet(m_presentDSLayout.get());
        if (!m_presentDescriptorSet) {
            LOG_ERROR("PathTracingPipeline", "重建 Present 描述符集失败");
            return false;
        }
        m_presentDescriptorSet->BindTexture(0, m_storageTexture.get(), m_presentSampler.get());
        m_presentDescriptorSet->Update();
    }

    LOG_DEBUG("PathTracingPipeline", "尺寸资源重建完成 ({}x{})", m_width, m_height);
    return true;
}

void PathTracingPipeline::BuildBVH() {
    int totalTris = m_cachedTriangleData.triangleCount;
    m_bvhNodeCount = 0;
    if (totalTris <= 0) return;

    // 1. 三角形索引 + 重心数组（用于节点分割）
    struct TriInfo { float cx, cy, cz; };
    std::vector<TriInfo> triInfos(totalTris);
    std::vector<int> triOrder(totalTris);
    for (int i = 0; i < totalTris; i++) {
        auto& tri = m_cachedTriangleData.triangles[i];
        triInfos[i] = {(tri.vertices[0].pos[0] + tri.vertices[1].pos[0] + tri.vertices[2].pos[0]) / 3.0f,
                       (tri.vertices[0].pos[1] + tri.vertices[1].pos[1] + tri.vertices[2].pos[1]) / 3.0f,
                       (tri.vertices[0].pos[2] + tri.vertices[1].pos[2] + tri.vertices[2].pos[2]) / 3.0f};
        triOrder[i] = i;
    }

    // 小场景（< 256 三角形）不做树分割，退化单叶节点避免 GPU 线程发散
    if (totalTris < 256) {
        m_bvhNodes.resize(1);
        auto& root = m_bvhNodes[0];
        root.aabbMin[0] = root.aabbMin[1] = root.aabbMin[2] = 1e30f;
        root.aabbMax[0] = root.aabbMax[1] = root.aabbMax[2] = -1e30f;
        for (int i = 0; i < totalTris; i++) {
            auto& tri = m_cachedTriangleData.triangles[i];
            for (int v = 0; v < 3; v++) {
                float* vert = (v == 0) ? tri.vertices[0].pos : (v == 1) ? tri.vertices[1].pos : tri.vertices[2].pos;
                root.aabbMin[0] = (std::min)(root.aabbMin[0], vert[0]);
                root.aabbMin[1] = (std::min)(root.aabbMin[1], vert[1]);
                root.aabbMin[2] = (std::min)(root.aabbMin[2], vert[2]);
                root.aabbMax[0] = (std::max)(root.aabbMax[0], vert[0]);
                root.aabbMax[1] = (std::max)(root.aabbMax[1], vert[1]);
                root.aabbMax[2] = (std::max)(root.aabbMax[2], vert[2]);
            }
        }
        root.aabbMin[3] = (float)totalTris;
        root.aabbMax[3] = 0.0f;
        m_bvhNodeCount = 1;
        LOG_INFO("PathTracingPipeline", "BVH 退化单节点: {} 个三角形（场景尺寸小）", totalTris);
        return;
    }

    m_bvhNodes.resize(MAX_BVH_NODES);
    m_bvhNodeCount = 0;
    std::function<int(int, int)> buildNode = [&](int start, int count) -> int {
        int idx = m_bvhNodeCount++;
        auto& node = m_bvhNodes[idx];
        node.aabbMin[0] = node.aabbMin[1] = node.aabbMin[2] = 1e30f;
        node.aabbMax[0] = node.aabbMax[1] = node.aabbMax[2] = -1e30f;
        node.aabbMin[3] = 0.0f; node.aabbMax[3] = 0.0f;

        for (int i = 0; i < count; i++) {
            int ti = triOrder[start + i];
            auto& tri = m_cachedTriangleData.triangles[ti];
            for (int v = 0; v < 3; v++) {
                float* vert = (v == 0) ? tri.vertices[0].pos : (v == 1) ? tri.vertices[1].pos : tri.vertices[2].pos;
                node.aabbMin[0] = (std::min)(node.aabbMin[0], vert[0]);
                node.aabbMin[1] = (std::min)(node.aabbMin[1], vert[1]);
                node.aabbMin[2] = (std::min)(node.aabbMin[2], vert[2]);
                node.aabbMax[0] = (std::max)(node.aabbMax[0], vert[0]);
                node.aabbMax[1] = (std::max)(node.aabbMax[1], vert[1]);
                node.aabbMax[2] = (std::max)(node.aabbMax[2], vert[2]);
            }
        }

        if (count <= 4) {
            node.aabbMin[3] = (float)count;
            node.aabbMax[3] = (float)start;
            return idx;
        }

        float ex = node.aabbMax[0] - node.aabbMin[0];
        float ey = node.aabbMax[1] - node.aabbMin[1];
        float ez = node.aabbMax[2] - node.aabbMin[2];
        int axis = (ex >= ey && ex >= ez) ? 0 : (ey >= ez) ? 1 : 2;
        float mid = (node.aabbMin[axis] + node.aabbMax[axis]) * 0.5f;

        int left = start, right = start + count - 1;
        while (left <= right) {
            while (left <= right && (&triInfos[triOrder[left]].cx)[axis] < mid) left++;
            while (left <= right && (&triInfos[triOrder[right]].cx)[axis] >= mid) right--;
            if (left < right) { std::swap(triOrder[left], triOrder[right]); left++; right--; }
        }
        int split = left;
        if (split == start || split == start + count)
            split = start + count / 2;

        node.aabbMin[3] = 0.0f;
        buildNode(start, split - start);        // left = idx + 1
        node.aabbMax[3] = (float)m_bvhNodeCount; // right child index
        buildNode(split, start + count - split);
        return idx;
    };

    buildNode(0, totalTris);
    m_bvhNodeCount = (uint32_t)m_bvhNodeCount;

    // 3. 按 BVH 叶节点顺序重排三角形（depth-first 连续）
    std::vector<PTTriangle> reordered(totalTris);
    std::vector<int> oldToNew(totalTris);
    for (int i = 0; i < totalTris; i++) {
        reordered[i] = m_cachedTriangleData.triangles[triOrder[i]];
        oldToNew[triOrder[i]] = i;
    }
    std::memcpy(m_cachedTriangleData.triangles, reordered.data(), totalTris * sizeof(PTTriangle));

    // 4. 重建 triToObject 映射（使用重排后的索引）
    m_triToObject.assign(totalTris, -1);
    for (uint32_t oi = 0; oi < (uint32_t)m_cachedSceneData.objectCount; oi++) {
        auto& obj = m_cachedSceneData.objects[oi];
        int type = (int)obj.p0[3];
        if (type == 4) {
            int oldFirst = (int)obj.p1[0];
            int triCnt = (int)obj.p1[1];
            for (int t = 0; t < triCnt; t++) {
                int newIdx = oldToNew[oldFirst + t];
                if (newIdx < totalTris) m_triToObject[newIdx] = (int)oi;
            }
            obj.p1[0] = (float)oldToNew[oldFirst]; // 更新对象三角形偏移到新顺序
        }
    }

    // 5. 上传到 GPU（如果已创建）
    if (m_sceneSSBO)
        m_sceneSSBO->UpdateData(&m_cachedSceneData, sizeof(PathTracingSceneData), 0);
    if (m_triangleBuffer)
        m_triangleBuffer->UpdateData(&m_cachedTriangleData, sizeof(PathTracingTriangleData), 0);
    if (m_bvhBuffer)
        m_bvhBuffer->UpdateData(m_bvhNodes.data(), m_bvhNodeCount * sizeof(BVHNode), 0);
    if (m_triToObjectBuffer)
        m_triToObjectBuffer->UpdateData(m_triToObject.data(), (uint32_t)(totalTris * sizeof(int)), 0);

    LOG_INFO("PathTracingPipeline", "BVH 构建完成: {} 个节点, {} 个三角形",
             m_bvhNodeCount, totalTris);
}

void PathTracingPipeline::Execute(const RenderContext& ctx) {
    if (!m_initialized || !ctx.device || !ctx.commandBuffer) return;

    auto cmd = ctx.commandBuffer;
    bool headless = m_device->IsHeadless();

    // 延迟初始化：根据帧尺寸重建存储纹理和描述符集（不重建着色器/管线）
    if ((ctx.width != m_width || ctx.height != m_height) && ctx.width > 0 && ctx.height > 0) {
        m_width = ctx.width;
        m_height = ctx.height;
        if (!ResizeResources()) {
            LOG_ERROR("PathTracingPipeline", "调整尺寸后重建资源失败");
        }
        if (m_cachedSceneData.objectCount > 0 && m_sceneSSBO) {
            m_sceneSSBO->UpdateData(&m_cachedSceneData, sizeof(PathTracingSceneData), 0);
        }
        if (m_cachedTriangleData.triangleCount > 0 && m_triangleBuffer) {
            m_triangleBuffer->UpdateData(&m_cachedTriangleData, sizeof(PathTracingTriangleData), 0);
        }
        if (m_bvhNodeCount > 0 && m_bvhBuffer) {
            m_bvhBuffer->UpdateData(m_bvhNodes.data(), m_bvhNodeCount * sizeof(BVHNode), 0);
        }
        if (!m_triToObject.empty() && m_triToObjectBuffer) {
            m_triToObjectBuffer->UpdateData(m_triToObject.data(), (uint32_t)(m_triToObject.size() * sizeof(int)), 0);
        }
        // 重建纹理后 accumImage 已归零，必须重置帧计数从 0 开始累积
        ResetAccumulation();
    }

    if (!m_storageTexture) return;
    // HardwareRT 模式不依赖计算管线，检查模式标记跳过此判断
    if (m_mode != PathTraceMode::HardwareRT && !m_computePipeline) return;

    // 每帧更新场景对象的 worldMatrix（本地空间 → 世界空间变换）
    // 累积阶段（m_frameCount > 0）场景和相机静止，无需每帧上传 SSBO
    if (m_scene && m_frameCount == 0) {
        UpdateTransforms(m_scene);
    }

    if (m_converged) {
        if (!headless) {
            m_device->BeginSwapChainRenderPass();
            cmd->SetViewport(Viewport{0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 1.0f});
            cmd->SetScissorRect(Rect{0, 0, (int)m_width, (int)m_height});
            if (m_presentPSO && m_presentDescriptorSet) {
                cmd->SetPipelineState(m_presentPSO.get());
                cmd->BindDescriptorSet(0, m_presentDescriptorSet.get());
                cmd->Draw(3, 1);
            }
            RenderOverlay(cmd);
            if (m_overlayCB) m_overlayCB(cmd);
        }
        return;
    }

    if (!headless) {
        m_device->EndSwapChainRenderPass();
    }

    // ===== 每帧 UBO 填充（两模式共用） =====
    glm::vec3 right = glm::vec3(ctx.camera.viewMatrix[0][0], ctx.camera.viewMatrix[1][0], ctx.camera.viewMatrix[2][0]);
    glm::vec3 up    = glm::vec3(ctx.camera.viewMatrix[0][1], ctx.camera.viewMatrix[1][1], ctx.camera.viewMatrix[2][1]);
    glm::vec3 dir   = -glm::vec3(ctx.camera.viewMatrix[0][2], ctx.camera.viewMatrix[1][2], ctx.camera.viewMatrix[2][2]);
    glm::vec3 pos = ctx.camera.position;

    PathTracingCameraUBO ubo;
    std::memcpy(ubo.cameraPos, &pos, sizeof(float) * 3);
    std::memcpy(ubo.cameraDir, &dir, sizeof(float) * 3);
    std::memcpy(ubo.cameraUp, &up, sizeof(float) * 3);
    std::memcpy(ubo.cameraRight, &right, sizeof(float) * 3);
    ubo.fov = ctx.camera.fov;
    ubo.aspectRatio = (float)m_width / (float)m_height;
    ubo.frameCount = (int)m_frameCount;
    ubo.maxBounces = (int)m_maxBounces;
    ubo.resetAccumulation = m_resetAccumulation ? 1 : 0;
    ubo.enableNEE = m_enableNEE ? 1 : 0;
    m_cameraUBO->UpdateData(&ubo, sizeof(ubo), 0);
    m_resetAccumulation = false;

    // ===== 按模式分派 =====
    if (m_mode == PathTraceMode::HardwareRT) {
        if (!m_rtResourcesBuilt && m_sceneChangedSinceLastRTBuild) {
            LOG_WARN("PathTracingPipeline", "RT 资源未就绪，尝试构建...");
            LoadRTHardwareShaders();
            BuildRTResources(m_scene);
        }

        // 屏障: Undefined/ShaderRead → UnorderedAccess
        {
            ResourceState ps = m_textureInitialized ? ResourceState::ShaderRead : ResourceState::Undefined;
            cmd->PipelineBarrier({{ m_storageTexture.get(), ps, ResourceState::UnorderedAccess }});
            m_textureInitialized = true;
        }

        ExecuteHardwareRT(cmd);

        // 写入 → ShaderRead 供 present
        cmd->PipelineBarrier({{ m_storageTexture.get(), ResourceState::UnorderedAccess, ResourceState::ShaderRead }});
    } else {
        // Compute 模式（Flat/BVH）
        {
            ResourceState ps = m_textureInitialized ? ResourceState::ShaderRead : ResourceState::Undefined;
            cmd->PipelineBarrier({{ m_storageTexture.get(), ps, ResourceState::UnorderedAccess }});
            m_textureInitialized = true;
        }

        cmd->SetComputePipeline(m_computePipeline.get());
        cmd->BindDescriptorSet(0, m_descriptorSet.get());

        uint32_t cw = m_width / 2, ch = m_height / 2;
        cmd->Dispatch((cw + 7) / 8, (ch + 7) / 8, 1);

        cmd->PipelineBarrier({{ m_storageTexture.get(), ResourceState::UnorderedAccess, ResourceState::ShaderRead }});
    }

    // 收敛检测
    if (m_maxSamples > 0 && m_frameCount >= m_maxSamples) {
        m_converged = true;
    }
    m_frameCount++;

    // 打开 swapchain RP 画全屏四边形（无头模式跳过）
    if (!headless) {
        m_device->BeginSwapChainRenderPass();
        cmd->SetViewport(Viewport{0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 1.0f});
        cmd->SetScissorRect(Rect{0, 0, (int)m_width, (int)m_height});
        if (m_presentPSO && m_presentDescriptorSet) {
            cmd->SetPipelineState(m_presentPSO.get());
            cmd->BindDescriptorSet(0, m_presentDescriptorSet.get());
            cmd->Draw(3, 1);
        }

        // Gizmo + 应用 HUD
        RenderOverlay(cmd);
        if (m_overlayCB) m_overlayCB(cmd);
    }
}

void PathTracingPipeline::SetSceneData(const PathTracingSceneData& data) {
    m_cachedSceneData = data;
    if (m_sceneSSBO) {
        m_sceneSSBO->UpdateData(&data, sizeof(data), 0);
    }
}

void PathTracingPipeline::SetTriangleData(const PathTracingTriangleData& data) {
    m_cachedTriangleData = data;
    if (m_triangleBuffer) {
        m_triangleBuffer->UpdateData(&data, sizeof(data), 0);
    }
}

void PathTracingPipeline::BuildFromScene(Scene* scene) {
    if (!scene) return;

    m_cachedSceneData = {};
    // 2.3MB 数组不能使用栈上临时对象（= {}），改用直接 memset
    memset(&m_cachedTriangleData, 0, sizeof(m_cachedTriangleData));
    m_scene = scene;

    uint32_t triOffset = 0;
    uint32_t objectIdx = 0;
    const uint32_t maxObjects = 32;

    for (const auto& node : scene->GetNodes()) {
        if (objectIdx >= maxObjects) break;

        auto meshRenderer = scene->GetComponent<Graphic::MeshRenderer>(node);
        if (!meshRenderer) continue;

        auto mesh = meshRenderer->GetMesh();
        if (!mesh || !mesh->HasCPUMeshData()) continue;

        auto emissive = meshRenderer->GetEmissive();
        float r = 0.7f, g = 0.7f, b = 0.7f;
        if (auto material = meshRenderer->GetMaterial()) {
            if (auto* baseColor = material->GetParam("BaseColor")) {
                if (auto* c = std::get_if<PrismaMath::vec4>(baseColor)) {
                    r = c->r; g = c->g; b = c->b;
                }
            }
        }

        // 顶点上传到本地空间，由 shader 每帧通过 worldMatrix 变换到世界空间
        uint32_t totalTris = 0;
        for (const auto& cpuMesh : mesh->GetCPUSubMeshes()) {
            const auto& indices = cpuMesh.indices;
            const auto& positions = cpuMesh.positions;
            const auto& normals = cpuMesh.normals;
            if (indices.empty() || positions.empty()) continue;

            uint32_t triCount = (uint32_t)(indices.size() / 3);

            for (uint32_t ti = 0; ti < triCount; ti++) {
                if (triOffset + ti >= PathTracingTriangleData::MAX_TRIANGLES) break;

                uint32_t i0 = indices[ti * 3 + 0];
                uint32_t i1 = indices[ti * 3 + 1];
                uint32_t i2 = indices[ti * 3 + 2];

                // 本地空间顶点（不上世界变换，shader 中每帧通过 worldMatrix 转换）
                glm::vec3 v0 = glm::vec3(positions[i0]);
                glm::vec3 v1 = glm::vec3(positions[i1]);
                glm::vec3 v2 = glm::vec3(positions[i2]);

                auto& tri = m_cachedTriangleData.triangles[triOffset + ti];
                // 写入 4 个 float 匹配 GPU std430 的 vec3 对齐（16 字节/顶点）
                std::memcpy(tri.vertices[0].pos, &v0, sizeof(float) * 3);
                tri.vertices[0].pos[3] = 0.0f;
                std::memcpy(tri.vertices[1].pos, &v1, sizeof(float) * 3);
                tri.vertices[1].pos[3] = 0.0f;
                std::memcpy(tri.vertices[2].pos, &v2, sizeof(float) * 3);
                tri.vertices[2].pos[3] = 0.0f;

                // 写入顶点法线（来自 OBJ vn，或为空时用面法线在 shader 中自动计算）
                if (!normals.empty() && i0 < normals.size() && i1 < normals.size() && i2 < normals.size()) {
                    glm::vec3 n0 = glm::vec3(normals[i0]);
                    glm::vec3 n1 = glm::vec3(normals[i1]);
                    glm::vec3 n2 = glm::vec3(normals[i2]);
                    std::memcpy(tri.vertices[0].nrm, &n0, sizeof(float) * 3);
                    tri.vertices[0].nrm[3] = 0.0f;
                    std::memcpy(tri.vertices[1].nrm, &n1, sizeof(float) * 3);
                    tri.vertices[1].nrm[3] = 0.0f;
                    std::memcpy(tri.vertices[2].nrm, &n2, sizeof(float) * 3);
                    tri.vertices[2].nrm[3] = 0.0f;
                }
            }

            totalTris += triCount;
        }

        if (totalTris == 0) continue;

        // 创建 PTSceneObject（type=4 = 三角形网格）
        auto& obj = m_cachedSceneData.objects[objectIdx];
        obj.p0[0] = 0.0f; obj.p0[1] = 0.0f; obj.p0[2] = 0.0f; obj.p0[3] = 4.0f;
        obj.p1[0] = (float)triOffset;
        obj.p1[1] = (float)totalTris;
        obj.p1[2] = 0.0f; obj.p1[3] = 0.0f;
        obj.p2[0] = 0.0f; obj.p2[1] = 0.0f; obj.p2[2] = 0.0f; obj.p2[3] = 0.0f;
        obj.color[0] = r; obj.color[1] = g; obj.color[2] = b;
        obj.color[3] = emissive.x + emissive.y + emissive.z;

        // worldMatrix 将每帧由 UpdateTransforms 填充，初始为 identity
        Matrix4x4 identity(1.0f);
        std::memcpy(obj.worldMatrix, &identity, sizeof(float) * 16);
        std::memcpy(obj.invWorldMatrix, &identity, sizeof(float) * 16);

        // 记录 node handle 用于每帧 transform 更新
        m_cachedNodeHandles[objectIdx] = node.handle;

        triOffset += totalTris;
        objectIdx++;
    }

    m_cachedSceneData.objectCount = (int)objectIdx;
    m_cachedTriangleData.triangleCount = (int)triOffset;

    // 上传到 GPU
    if (m_sceneSSBO) {
        m_sceneSSBO->UpdateData(&m_cachedSceneData, sizeof(PathTracingSceneData), 0);
    }
    if (m_triangleBuffer) {
        m_triangleBuffer->UpdateData(&m_cachedTriangleData, sizeof(PathTracingTriangleData), 0);
    }

    LOG_INFO("PathTracingPipeline", "BuildFromScene: {} 个对象, {} 个三角形",
             objectIdx, triOffset);
}

void PathTracingPipeline::UpdateTransforms(Scene* scene) {
    if (!scene || !m_sceneSSBO || m_cachedSceneData.objectCount == 0) return;

    for (int i = 0; i < m_cachedSceneData.objectCount; i++) {
        Node node(m_cachedNodeHandles[i]);
        if (!node.IsValid()) continue;

        Matrix4x4 worldMat = scene->GetWorldTransform(node);
        std::memcpy(m_cachedSceneData.objects[i].worldMatrix, &worldMat, sizeof(float) * 16);
        // 同步计算逆矩阵（供局部空间射线追踪使用）
        glm::mat4 wm;
        std::memcpy(&wm, &worldMat, sizeof(float) * 16);
        glm::mat4 invWm = glm::inverse(wm);
        std::memcpy(m_cachedSceneData.objects[i].invWorldMatrix, &invWm, sizeof(float) * 16);
    }

    m_sceneSSBO->UpdateData(&m_cachedSceneData, sizeof(PathTracingSceneData), 0);
}

void PathTracingPipeline::ResetAccumulation() {
    m_frameCount = 0;
    m_converged = false;
    m_resetAccumulation = true;
}

// 半精度浮点数 (FP16) → float 转换
static float HalfToFloat(uint16_t h) {
    uint32_t sign = (uint32_t)((h >> 15) & 1) << 31;
    uint32_t exp  = (h >> 10) & 0x1f;
    uint32_t mant = h & 0x3ff;
    if (exp == 0) {
        // 零或非规格化数
        if (mant == 0) return std::bit_cast<float>(sign);
        // 非规格化: 左移归一化
        while ((mant & 0x400) == 0) { mant <<= 1; exp--; }
        mant &= 0x3ff;
        exp += 112; // 从偏置 15 重偏置到 127
    } else if (exp == 0x1f) {
        exp = 0xff; // Inf/NaN
    } else {
        exp += 112;
    }
    return std::bit_cast<float>(sign | (exp << 23) | (mant << 13));
}

bool PathTracingPipeline::SaveOutput(const std::string& path) {
    if (!m_cameraUBO || !m_storageTexture) {
        LOG_ERROR("PathTracingPipeline", "无数据可保存");
        return false;
    }
    LOG_INFO("PathTracingPipeline", "保存输出到: {} (帧数: {})", path, m_frameCount);
    m_device->WaitForIdle();

    uint32_t w = m_width / 2;
    uint32_t h = m_height / 2;

    // 纹理是 RGBA16_Float (8 bytes/pixel)，以原始字节形式回读
    size_t rawSize = (size_t)w * h * 8;
    std::vector<uint8_t> rawData(rawSize);
    if (!m_device->ReadbackTexture(m_storageTexture.get(), w, h, rawData.data(), rawSize)) {
        LOG_ERROR("PathTracingPipeline", "GPU 纹理回读失败");
        return false;
    }

    // 从 half4 → float4 → 写 PNG (u8)
    std::vector<uint8_t> rgba8(w * h * 4);
    const uint16_t* halfPixels = reinterpret_cast<const uint16_t*>(rawData.data());
    for (size_t i = 0; i < (size_t)w * h; i++) {
        for (int c = 0; c < 4; c++) {
            float v = glm::clamp(HalfToFloat(halfPixels[i * 4 + c]), 0.0f, 1.0f);
            rgba8[i * 4 + c] = (uint8_t)(v * 255.0f + 0.5f);
        }
    }

    bool saved = Utils::ImageUtils::SavePNG(path, (int)w, (int)h, 4, rgba8.data(), (int)w * 4);

    if (saved) {
        LOG_INFO("PathTracingPipeline", "输出已保存: {} ({}x{}, {} 帧)", path, w, h, m_frameCount);
    } else {
        LOG_ERROR("PathTracingPipeline", "保存 PNG 失败: {}", path);
    }
    return saved;
}

void PathTracingPipeline::Shutdown() {
    if (!m_initialized) return;
    DestroyResources();
    m_initialized = false;
}

void PathTracingPipeline::SetMode(PathTraceMode newMode) {
    if (!m_initialized || !m_device || newMode == m_mode) return;
    m_targetMode = newMode;
    m_device->WaitForIdle();

    auto* factory = m_device->GetResourceFactory();
    if (!factory) return;

    // ===== 离开当前模式 → 清理旧资源 =====
    if (m_mode == PathTraceMode::HardwareRT) {
        DestroyRTResources();
    }
    if (newMode != PathTraceMode::HardwareRT) {
        // 现有模式间切换：重建计算管线
        m_computeShader.reset();
        m_descriptorSet.reset();
        m_computePipeline.reset();

        // Flat↔BVH 数据转换
        if (newMode == PathTraceMode::BVH && m_bvhNodeCount == 0 && m_cachedTriangleData.triangleCount > 0) {
            // Flat→BVH：预变换顶点到世界空间
            for (uint32_t oi = 0; oi < (uint32_t)m_cachedSceneData.objectCount; oi++) {
                auto& obj = m_cachedSceneData.objects[oi];
                int type = (int)obj.p0[3];
                if (type != 4) continue;
                int firstTri = (int)obj.p1[0];
                int triCnt = (int)obj.p1[1];
                glm::mat4 worldMat;
                std::memcpy(&worldMat, obj.worldMatrix, sizeof(float) * 16);
                for (int t = 0; t < triCnt; t++) {
                    auto& tri = m_cachedTriangleData.triangles[firstTri + t];
                    glm::vec4 v0 = worldMat * glm::vec4(tri.vertices[0].pos[0], tri.vertices[0].pos[1], tri.vertices[0].pos[2], 1.0f);
                    glm::vec4 v1 = worldMat * glm::vec4(tri.vertices[1].pos[0], tri.vertices[1].pos[1], tri.vertices[1].pos[2], 1.0f);
                    glm::vec4 v2 = worldMat * glm::vec4(tri.vertices[2].pos[0], tri.vertices[2].pos[1], tri.vertices[2].pos[2], 1.0f);
                    tri.vertices[0].pos[0] = v0.x; tri.vertices[0].pos[1] = v0.y; tri.vertices[0].pos[2] = v0.z;
                    tri.vertices[1].pos[0] = v1.x; tri.vertices[1].pos[1] = v1.y; tri.vertices[1].pos[2] = v1.z;
                    tri.vertices[2].pos[0] = v2.x; tri.vertices[2].pos[1] = v2.y; tri.vertices[2].pos[2] = v2.z;
                }
            }
            BuildBVH();
        } else if (newMode == PathTraceMode::Flat && m_scene) {
            // BVH→Flat：从场景重建本地空间数据
            m_bvhNodes.clear();
            m_bvhNodeCount = 0;
            m_triToObject.clear();
            BuildFromScene(m_scene);
        }

        // 重新创建计算管线
        LoadDefaultShaders();
        m_computePipeline = factory->CreateComputePipelineImpl();
        if (!m_computePipeline) { LOG_ERROR("PathTracingPipeline", "切换模式时创建管线失败"); return; }
        if (m_computeShader) m_computePipeline->SetShader(m_computeShader);
        if (!m_computePipeline->Create(m_device)) { LOG_ERROR("PathTracingPipeline", "切换模式时编译管线失败"); return; }

        auto layoutPtrs = m_computePipeline->GetDescriptorSetLayouts();
        if (layoutPtrs.empty()) return;
        m_descriptorSet = factory->CreateDescriptorSet(layoutPtrs[0].get());
        if (!m_descriptorSet) return;
        m_descriptorSet->BindStorageImage(0, m_storageTexture.get());
        m_descriptorSet->BindStorageImage(1, m_storageTexture.get());
        m_descriptorSet->BindBuffer(2, m_cameraUBO.get(), 0, sizeof(PathTracingCameraUBO), DescriptorType::UniformBuffer);
        m_descriptorSet->BindBuffer(3, m_sceneSSBO.get(), 0, sizeof(PathTracingSceneData), DescriptorType::StorageBuffer);
        m_descriptorSet->BindBuffer(4, m_triangleBuffer.get(), 0, sizeof(PathTracingTriangleData), DescriptorType::StorageBuffer);
        m_descriptorSet->BindBuffer(5, m_bvhBuffer.get(), 0, MAX_BVH_NODES * sizeof(BVHNode), DescriptorType::StorageBuffer);
        m_descriptorSet->BindBuffer(6, m_triToObjectBuffer.get(), 0, PathTracingTriangleData::MAX_TRIANGLES * sizeof(int), DescriptorType::StorageBuffer);
        m_descriptorSet->Update();

        // 重新上传场景数据
        if (m_cachedSceneData.objectCount > 0 && m_sceneSSBO)
            m_sceneSSBO->UpdateData(&m_cachedSceneData, sizeof(PathTracingSceneData), 0);
        if (m_cachedTriangleData.triangleCount > 0 && m_triangleBuffer)
            m_triangleBuffer->UpdateData(&m_cachedTriangleData, sizeof(PathTracingTriangleData), 0);
        if (m_bvhNodeCount > 0 && m_bvhBuffer)
            m_bvhBuffer->UpdateData(m_bvhNodes.data(), m_bvhNodeCount * sizeof(BVHNode), 0);
        if (!m_triToObject.empty() && m_triToObjectBuffer)
            m_triToObjectBuffer->UpdateData(m_triToObject.data(), (uint32_t)(m_triToObject.size() * sizeof(int)), 0);
    }

    // ===== 进入新模式 =====
    if (newMode == PathTraceMode::HardwareRT) {
        if (!m_device->SupportsRayTracing()) {
            LOG_WARN("PathTracingPipeline", "设备不支持光线追踪，回退 BVH");
            m_mode = PathTraceMode::BVH;
            m_targetMode = m_mode;
            return;
        }
        // 松开计算管线资源（减少内存占用）
        m_computePipeline.reset();
        m_descriptorSet.reset();
        m_computeShader.reset();

        // 构建 RT 资源
        LoadRTHardwareShaders();
        BuildRTResources(m_scene);
    }

    m_mode = newMode;
    m_targetMode = m_mode;
    ResetAccumulation();
    LOG_INFO("PathTracingPipeline", "切换模式: {}", GetModeName());
}

void PathTracingPipeline::CycleMode() {
    switch (m_mode) {
        case PathTraceMode::Flat:       SetMode(PathTraceMode::BVH); break;
        case PathTraceMode::BVH:        SetMode(PathTraceMode::HardwareRT); break;
        case PathTraceMode::HardwareRT: SetMode(PathTraceMode::Flat); break;
    }
}

const char* PathTracingPipeline::GetModeName() const {
    switch (m_mode) {
        case PathTraceMode::Flat:       return "Flat";
        case PathTraceMode::BVH:        return "BVH";
        case PathTraceMode::HardwareRT: return "HardwareRT";
    }
    return "Unknown";
}

void PathTracingPipeline::DestroyResources() {
    DestroyRTResources();
    m_descriptorSet.reset();
    m_computePipeline.reset();
    m_triangleBuffer.reset();
    m_bvhBuffer.reset();
    m_triToObjectBuffer.reset();
    m_sceneSSBO.reset();
    m_cameraUBO.reset();
    m_storageTexture.reset();
    m_textureInitialized = false;

    m_presentDescriptorSet.reset();
    m_presentSampler.reset();
    m_presentDSLayout.reset();
    m_presentPSO.reset();
    // 注意：不重置 m_presentVertShader / m_presentFragShader
    // 着色器 VkShaderModule 不依赖窗口尺寸，Destroy+Create 期间可复用
    // 如果此处被释放，后续 CreateResources 会因 SPIRV 数据为空而失败

    m_gizmoPSO.reset();
    m_gizmoVertShader.reset();
    m_gizmoFragShader.reset();
    m_gizmoCamera.reset();

    LOG_DEBUG("PathTracingPipeline", "资源已清理");
}

void PathTracingPipeline::LoadDefaultShaders() {
    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rm) {
        LOG_ERROR("PathTracingPipeline", "无法获取 RenderResourceManager，默认着色器加载失败");
        return;
    }

    if (!m_computeShader && m_computeSPIRV.empty()) {
        const char* shaderPath = (m_mode == PathTraceMode::BVH)
            ? "assets/shaders/pathtrace_BVH.comp.spv"
            : "assets/shaders/pathtrace.comp.spv";
        auto shader = rm->LoadShaderSync(shaderPath);
        if (shader) {
            SetComputeShader(std::move(shader));
            LOG_INFO("PathTracingPipeline", "内部加载计算着色器: {} {}",
                     shaderPath, m_mode == PathTraceMode::BVH ? "(BVH)" : "(Flat)");
        }
    }

    if (!m_presentVertShader && m_presentVertSPIRV.empty()) {
        auto vert = rm->LoadShaderSync("assets/shaders/fullscreen.vert.spv");
        auto frag = rm->LoadShaderSync("assets/shaders/present.frag.spv");
        if (vert && frag) {
            SetPresentShaders(std::move(vert), std::move(frag));
            LOG_INFO("PathTracingPipeline", "内部加载 present 着色器");
        }
    }
}

bool PathTracingPipeline::LoadRTHardwareShaders() {
    auto readFile = [](const std::string& path) -> std::vector<uint8_t> {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) return {};
        size_t size = (size_t)file.tellg();
        file.seekg(0);
        std::vector<uint8_t> data(size);
        file.read((char*)data.data(), size);
        return data;
    };

    // SPIR-V 文件路径（相对于引擎资源目录）
    std::string base = "assets/shaders/pathtrace_HardwareRT";
    m_rgenSPIRV = readFile(base + ".rgen.spv");
    m_rchitSPIRV = readFile(base + ".rchit.spv");
    m_rmissSPIRV = readFile(base + ".rmiss.spv");

    if (m_rgenSPIRV.empty() || m_rchitSPIRV.empty() || m_rmissSPIRV.empty()) {
        LOG_ERROR("PathTracingPipeline", "RT 着色器加载失败: {}.rgen/rchit/rmiss.spv", base);
        return false;
    }
    LOG_INFO("PathTracingPipeline", "RT 着色器加载成功: rgen={}B rchit={}B rmiss={}B",
             m_rgenSPIRV.size(), m_rchitSPIRV.size(), m_rmissSPIRV.size());
    return true;
}

bool PathTracingPipeline::BuildRTResources([[maybe_unused]] Scene* scene) {
    if (!m_device || !m_device->SupportsRayTracing()) {
        LOG_ERROR("PathTracingPipeline", "设备不支持光线追踪");
        return false;
    }

    // 1. 初始化 RT 后端
    if (!m_rtBackend) {
        m_rtBackend = std::make_unique<VulkanRTBackend>();
        VkDevice vkDev = m_device->GetVkDevice();
        VkPhysicalDevice physDev = m_device->GetPhysicalDevice();
        VmaAllocator allocator = m_device->GetVmaAllocator();
        uint32_t gfxQF = m_device->GetGraphicsQueueFamily();
        if (!m_rtBackend->Initialize(vkDev, physDev, allocator, gfxQF)) {
            LOG_ERROR("PathTracingPipeline", "RT 后端初始化失败");
            m_rtBackend.reset();
            return false;
        }
    }

    // 2. 构建每个网格对象的 BLAS
    m_rtBackend->m_blasEntries.clear();
    for (uint32_t oi = 0; oi < (uint32_t)m_cachedSceneData.objectCount; oi++) {
        auto& obj = m_cachedSceneData.objects[oi];
        int type = (int)obj.p0[3];
        if (type != 4) continue; // 只有网格对象有 BLAS
        int firstTri = (int)obj.p1[0];
        int triCnt = (int)obj.p1[1];
        if (triCnt == 0) continue;

        // 收集三角形顶点到平面数组
        std::vector<float> verts;
        std::vector<uint32_t> indices;
        for (int t = 0; t < triCnt; t++) {
            auto& tri = m_cachedTriangleData.triangles[firstTri + t];
            for (int v = 0; v < 3; v++) {
                verts.push_back(tri.vertices[v].pos[0]);
                verts.push_back(tri.vertices[v].pos[1]);
                verts.push_back(tri.vertices[v].pos[2]);
            }
            indices.push_back(t * 3 + 0);
            indices.push_back(t * 3 + 1);
            indices.push_back(t * 3 + 2);
        }

        VulkanRTBackend::BLASInput input;
        input.vertices = verts.data();
        input.vertexCount = (uint32_t)(verts.size() / 3);
        input.indices = indices.data();
        input.indexCount = (uint32_t)indices.size();

        VkAccelerationStructureKHR blas = m_rtBackend->BuildBLAS(input);
        if (blas == VK_NULL_HANDLE) {
            LOG_WARN("PathTracingPipeline", "对象 {} BLAS 构建失败", oi);
            continue;
        }
    }

    if (m_rtBackend->m_blasEntries.empty()) {
        LOG_ERROR("PathTracingPipeline", "没有成功构建任何 BLAS");
        return false;
    }

    // 3. 构建 TLAS（mesh 对象按创建顺序对应 BLAS 条目）
    std::vector<VulkanRTBackend::InstanceInput> instances;
    uint32_t blasIdx = 0;
    for (uint32_t oi = 0; oi < (uint32_t)m_cachedSceneData.objectCount; oi++) {
        auto& obj = m_cachedSceneData.objects[oi];
        int type = (int)obj.p0[3];
        if (type != 4) continue;

        if (blasIdx >= (uint32_t)m_rtBackend->m_blasEntries.size()) break;
        uint64_t blasAddr = m_rtBackend->GetBLASDeviceAddress(
            m_rtBackend->m_blasEntries[blasIdx].handle);
        blasIdx++;
        if (blasAddr == 0) continue;

        // 世界变换矩阵（行主序 → VkTransformMatrixKHR 列主序）
        glm::mat4 wm;
        std::memcpy(&wm, obj.worldMatrix, sizeof(float) * 16);
        VkTransformMatrixKHR transform = {{
            { wm[0][0], wm[1][0], wm[2][0], wm[3][0] },
            { wm[0][1], wm[1][1], wm[2][1], wm[3][1] },
            { wm[0][2], wm[1][2], wm[2][2], wm[3][2] }
        }};

        VulkanRTBackend::InstanceInput inst;
        inst.transform = transform;
        inst.instanceCustomIndex = (int)oi;
        inst.blasDeviceAddress = blasAddr;
        instances.push_back(inst);
    }

    if (!m_rtBackend->BuildTLAS(instances)) {
        LOG_ERROR("PathTracingPipeline", "TLAS 构建失败");
        return false;
    }

    // 4. 通过 RHI 创建描述符集布局和描述符集（不再绕过 RHI）
    auto* factory = m_device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("PathTracingPipeline", "无法获取资源工厂");
        return false;
    }

    std::vector<ShaderResource> rtResources = {
        {"outputImage", ShaderResource::Type::StorageImage, 0, 0, 1, 0},
        {"accumImage",  ShaderResource::Type::StorageImage, 0, 1, 1, 0},
        {"cameraUBO",   ShaderResource::Type::UniformBuffer, 0, 2, 1, sizeof(PathTracingCameraUBO)},
        {"sceneSSBO",   ShaderResource::Type::StorageBuffer, 0, 3, 1, sizeof(PathTracingSceneData)},
        {"triangleBuf", ShaderResource::Type::StorageBuffer, 0, 4, 1, sizeof(PathTracingTriangleData)},
        {"tlas",        ShaderResource::Type::AccelerationStructure, 0, 5, 1, 0},
    };
    auto rtDescLayout = factory->CreateDescriptorSetLayout(rtResources);
    if (!rtDescLayout) {
        LOG_ERROR("PathTracingPipeline", "创建 RT 描述符集布局失败");
        return false;
    }

    m_rtRhiDescriptorSet = factory->CreateDescriptorSet(rtDescLayout.get());
    if (!m_rtRhiDescriptorSet) {
        LOG_ERROR("PathTracingPipeline", "创建 RT 描述符集失败");
        return false;
    }

    m_rtRhiDescriptorSet->BindStorageImage(0, m_storageTexture.get());
    m_rtRhiDescriptorSet->BindStorageImage(1, m_storageTexture.get());
    m_rtRhiDescriptorSet->BindBuffer(2, m_cameraUBO.get(), 0, sizeof(PathTracingCameraUBO),
                                     DescriptorType::UniformBuffer);
    m_rtRhiDescriptorSet->BindBuffer(3, m_sceneSSBO.get(), 0, sizeof(PathTracingSceneData),
                                     DescriptorType::StorageBuffer);
    m_rtRhiDescriptorSet->BindBuffer(4, m_triangleBuffer.get(), 0, sizeof(PathTracingTriangleData),
                                     DescriptorType::StorageBuffer);
    m_rtRhiDescriptorSet->BindAccelerationStructure(5,
        reinterpret_cast<void*>(static_cast<uintptr_t>(m_rtBackend->GetTLAS())));
    m_rtRhiDescriptorSet->Update();

    // 5. 从 RHI 描述符集布局获取 Vulkan 原生布局，创建管线布局
    VkDescriptorSetLayout vkDescLayout = static_cast<VkDescriptorSetLayout>(
        rtDescLayout->GetNativeHandle());
    VkPipelineLayout vkPipeLayout = m_rtBackend->CreatePipelineLayout(vkDescLayout);
    if (vkPipeLayout == VK_NULL_HANDLE) {
        LOG_ERROR("PathTracingPipeline", "创建 RT 管线布局失败");
        return false;
    }

    // 6. 创建 RT 管线
    VulkanRTBackend::RTPipelineShaders shaders;
    shaders.rgenCode = m_rgenSPIRV.data();
    shaders.rgenSize = m_rgenSPIRV.size();
    shaders.rchitCode = m_rchitSPIRV.data();
    shaders.rchitSize = m_rchitSPIRV.size();
    shaders.rmissCode = m_rmissSPIRV.data();
    shaders.rmissSize = m_rmissSPIRV.size();

    if (!m_rtBackend->CreateRTPipeline(shaders, vkDescLayout, vkPipeLayout)) {
        LOG_ERROR("PathTracingPipeline", "创建 RT 管线失败");
        return false;
    }

    m_rtResourcesBuilt = true;
    m_sceneChangedSinceLastRTBuild = false;

    LOG_INFO("PathTracingPipeline", "RT 资源构建完成: {} 个 BLAS, {} 个实例",
             m_rtBackend->m_blasEntries.size(), instances.size());
    return true;
}

void PathTracingPipeline::DestroyRTResources() {
    if (!m_rtBackend) return;
    m_rtBackend->Shutdown();
    m_rtBackend.reset();
    m_rtRhiDescriptorSet.reset();
    m_rgenSPIRV.clear();
    m_rchitSPIRV.clear();
    m_rmissSPIRV.clear();
    m_rtResourcesBuilt = false;
    LOG_DEBUG("PathTracingPipeline", "RT 资源已清理");
}

void PathTracingPipeline::ExecuteHardwareRT(ICommandBuffer* cmd) {
    if (!m_rtBackend || !m_rtBackend->GetRTPipeline()) return;
    if (!m_rtRhiDescriptorSet) {
        LOG_ERROR("PathTracingPipeline", "ExecuteHardwareRT: RT 描述符集为空");
        return;
    }

    // 获取 VkCommandBuffer（如 RenderSystem.cpp 中的惯例）
    auto* vkCmdBuf = dynamic_cast<Vulkan::VulkanCommandBuffer*>(cmd);
    if (!vkCmdBuf) {
        LOG_ERROR("PathTracingPipeline", "ExecuteHardwareRT: 无法获取 VulkanCommandBuffer");
        return;
    }
    VkCommandBuffer vkCmd = vkCmdBuf->GetVkCommandBuffer();

    // 从 RHI 描述符集提取 Vulkan 原生句柄
    VkDescriptorSet vkDescSet = static_cast<VkDescriptorSet>(
        m_rtRhiDescriptorSet->GetNativeHandle());

    // 全分辨率（rgen 中每个 gl_LaunchIDEXT = 一个像素）
    uint32_t dispatchW = m_width / 2;
    uint32_t dispatchH = m_height / 2;

    m_rtBackend->BindAndTraceRays(vkCmd, dispatchW, dispatchH, vkDescSet);
}

void PathTracingPipeline::InitOverlayResources() {
    auto* factory = m_device->GetResourceFactory();
    if (!factory) return;

    // 内部自动加载 gizmo overlay 着色器（如未被外部设置）
    if (!m_gizmoVertShader || !m_gizmoFragShader) {
        auto* rm = Engine::Get().GetRenderResourceManager();
        if (rm) {
            m_gizmoVertShader = rm->LoadShaderSync("assets/shaders/Renderer2D.vert.spv");
            m_gizmoFragShader = rm->LoadShaderSync("assets/shaders/UnlitVertex.frag.spv");
        }
    }

    if (!m_gizmoVertShader || !m_gizmoFragShader) {
        LOG_WARN("PathTracingPipeline", "gizmo 着色器加载失败，跳过 overlay 初始化");
        return;
    }

    auto pso = factory->CreatePipelineStateImpl();
    pso->SetShader(ShaderType::Vertex, m_gizmoVertShader);
    pso->SetShader(ShaderType::Pixel, m_gizmoFragShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    RasterizerState rs{};
    rs.cullMode = CullMode::None;
    pso->SetRasterizerState(rs);
    if (pso->Create(m_device)) {
        m_gizmoPSO = std::shared_ptr<IPipelineState>(std::move(pso));
        LOG_INFO("PathTracingPipeline", "gizmo PSO 创建成功");
    } else {
        LOG_ERROR("PathTracingPipeline", "gizmo PSO 创建失败");
    }

    m_gizmoCamera = std::make_shared<OrthographicCamera>(
        0.0f, static_cast<float>(m_width), 0.0f, static_cast<float>(m_height)
    );
}

void PathTracingPipeline::RenderOverlay(ICommandBuffer* cmd) {
    const auto& gizmoCommands = Renderer::GetGizmoQueue();
    if (gizmoCommands.empty() || !m_gizmoPSO) return;

    cmd->SetPipelineState(m_gizmoPSO.get());

    float w = static_cast<float>(m_width);
    float h = static_cast<float>(m_height);
    cmd->SetViewport(Viewport{0.0f, 0.0f, w, h, 0.0f, 1.0f});
    cmd->SetScissorRect(Rect{0, 0, static_cast<int>(w), static_cast<int>(h)});

    // 每次渲染前更新正交投影匹配当前视口，确保 Overlay 不随窗口缩放变形
    if (m_gizmoCamera) {
        m_gizmoCamera->SetViewport(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    }
    auto vp = m_gizmoCamera ? m_gizmoCamera->GetViewProjectionMatrix() : glm::mat4(1.0f);

    for (const auto& gc : gizmoCommands) {
        if (!gc.mesh) continue;
        GizmoPushConstants pc{};
        pc.mvp = vp * gc.transform;
        pc.color = gc.color;
        cmd->PushConstants(ShaderType::Vertex, &pc, sizeof(pc));
        cmd->PushConstants(ShaderType::Pixel, &pc, sizeof(pc));

        for (const auto& subMesh : gc.mesh->GetSubMeshes()) {
            if (subMesh.vertexBuffer && subMesh.indexBuffer) {
                cmd->SetVertexBuffer(subMesh.vertexBuffer.get(), 0);
                cmd->SetIndexBuffer(subMesh.indexBuffer.get());
                cmd->DrawIndexed(subMesh.indexCount);
            }
        }
    }

    Renderer::ClearGizmoQueue();
}

} // namespace Prisma::Graphic
