#include "PathTracingPipeline.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/ISampler.h"
#include "graphic/RenderDesc.h"
#include "graphic/interfaces/ShaderReflection.h"
#include "Logger.h"
#include "utils/ImageUtils.h"
#include <glm/glm.hpp>
#include <cstring>
#include <fstream>
#include <sstream>

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
    if (m_computeSPIRV.empty() || m_presentVertSPIRV.empty() || m_presentFragSPIRV.empty()) {
        LOG_ERROR("PathTracingPipeline", "着色器 SPIR-V 数据未设置");
        return -1;
    }

    m_device = device;

    if (!CreateResources()) {
        LOG_ERROR("PathTracingPipeline", "创建资源失败");
        DestroyResources();
        return -1;
    }

    return 0;
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
    texDesc.format = TextureFormat::RGBA32_Float;
    texDesc.width = m_width ? m_width : 1280;
    texDesc.height = m_height ? m_height : 720;
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

    // 4. 创建计算着色器
    ShaderReflection reflection;
    reflection.Resources = {
        {"outputImage", ShaderResource::Type::Image2D, 0, 0, 1, 0},
        {"accumImage",  ShaderResource::Type::Image2D, 0, 1, 1, 0},
        {"cameraUBO",   ShaderResource::Type::UniformBuffer, 0, 2, 1, sizeof(PathTracingCameraUBO)},
        {"sceneSSBO",   ShaderResource::Type::StorageBuffer, 0, 3, 1, sizeof(PathTracingSceneData)},
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

    // 5. 创建计算管线
    m_computePipeline = factory->CreateComputePipelineImpl();
    if (!m_computePipeline) {
        LOG_ERROR("PathTracingPipeline", "创建计算管线对象失败");
        return false;
    }

    m_computePipeline->SetShader(std::move(computeShader));
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
    m_descriptorSet->Update();

    // 8. 创建 Present 着色器
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

    m_initialized = true;
    LOG_INFO("PathTracingPipeline", "路径追踪管线初始化完成 ({}x{})",
             m_width ? m_width : 1280, m_height ? m_height : 720);
    return true;
}

void PathTracingPipeline::Execute(const RenderContext& ctx) {
    if (!m_initialized || !ctx.device || !ctx.commandBuffer) return;

    auto cmd = ctx.commandBuffer;
    bool headless = m_device->IsHeadless();

    // 延迟初始化：根据帧尺寸创建资源
    if ((ctx.width != m_width || ctx.height != m_height) && ctx.width > 0 && ctx.height > 0) {
        m_width = ctx.width;
        m_height = ctx.height;
        DestroyResources();
        CreateResources();
        if (m_cachedSceneData.objectCount > 0 && m_sceneSSBO) {
            m_sceneSSBO->UpdateData(&m_cachedSceneData, sizeof(PathTracingSceneData), 0);
        }
        // 重建纹理后 accumImage 已归零，必须重置帧计数从 0 开始累积
        ResetAccumulation();
    }

    if (!m_storageTexture || !m_computePipeline) return;

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
            if (m_overlayCB) m_overlayCB(cmd);
        }
        return;
    }

    if (!headless) {
        m_device->EndSwapChainRenderPass();
    }

    cmd->PipelineBarrier({{
        m_storageTexture.get(),
        ResourceState::Undefined,
        ResourceState::UnorderedAccess
    }});

    // 填充 Camera UBO
    {
        glm::mat4 view = ctx.camera.viewMatrix;
        // 注意：glm::lookAt 生成的视图矩阵中，camera 基向量是按行排列的：
        //   view = [s.x  s.y  s.z  t.x]    s = right
        //          [u.x  u.y  u.z  t.y]    u = up
        //          [-f.x -f.y -f.z t.z]    f = forward
        //          [0    0    0    1   ]
        // GLM 列主序存储，所以 view[col][row]：
        //   列 0 = (s.x, u.x, -f.x), 列 1 = (s.y, u.y, -f.y), 列 2 = (s.z, u.z, -f.z)
        //   按行提取才能得到正确的基向量
        glm::vec3 right = glm::vec3(view[0][0], view[1][0], view[2][0]);
        glm::vec3 up    = glm::vec3(view[0][1], view[1][1], view[2][1]);
        glm::vec3 dir   = -glm::vec3(view[0][2], view[1][2], view[2][2]);
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
    }

    cmd->SetComputePipeline(m_computePipeline.get());
    cmd->BindDescriptorSet(0, m_descriptorSet.get());

    uint32_t groupX = (m_width + 7) / 8;
    uint32_t groupY = (m_height + 7) / 8;
    cmd->Dispatch(groupX, groupY, 1);

    // 管线屏障：存储图像转换到 ShaderRead 供 present
    cmd->PipelineBarrier({{
        m_storageTexture.get(),
        ResourceState::UnorderedAccess,
        ResourceState::ShaderRead
    }});

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

        // Overlay 回调（gizmo / HUD）
        if (m_overlayCB) m_overlayCB(cmd);
    }
}

void PathTracingPipeline::SetSceneData(const PathTracingSceneData& data) {
    m_cachedSceneData = data;
    if (m_sceneSSBO) {
        m_sceneSSBO->UpdateData(&data, sizeof(data), 0);
    }
}

void PathTracingPipeline::ResetAccumulation() {
    m_frameCount = 0;
    m_converged = false;
    m_resetAccumulation = true;
}

bool PathTracingPipeline::SaveOutput(const std::string& path) {
    if (!m_cameraUBO || !m_storageTexture) {
        LOG_ERROR("PathTracingPipeline", "无数据可保存");
        return false;
    }
    LOG_INFO("PathTracingPipeline", "保存输出到: {} (帧数: {})", path, m_frameCount);
    m_device->WaitForIdle();

    uint32_t w = m_width;
    uint32_t h = m_height;
    std::vector<float> pixelData(w * h * 4);

    if (!m_device->ReadbackTexture(m_storageTexture.get(), w, h, pixelData.data(), pixelData.size() * sizeof(float))) {
        LOG_ERROR("PathTracingPipeline", "GPU 纹理回读失败");
        return false;
    }

    // 写 PNG
    std::vector<uint8_t> rgba8(w * h * 4);
    for (size_t i = 0; i < (size_t)w * h; i++) {
        for (int c = 0; c < 4; c++) {
            float v = glm::clamp(pixelData[i * 4 + c], 0.0f, 1.0f);
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

void PathTracingPipeline::DestroyResources() {
    m_descriptorSet.reset();
    m_computePipeline.reset();
    m_sceneSSBO.reset();
    m_cameraUBO.reset();
    m_storageTexture.reset();

    m_presentDescriptorSet.reset();
    m_presentSampler.reset();
    m_presentDSLayout.reset();
    m_presentPSO.reset();
    m_presentVertShader.reset();
    m_presentFragShader.reset();

    LOG_DEBUG("PathTracingPipeline", "资源已清理");
}

} // namespace Prisma::Graphic
