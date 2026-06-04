#include "ClusteredForwardPipeline.h"
#include "ClusteredOpaquePass.h"
#include "../forward/DepthPrePass.h"
#include "../forward/TransparentPass.h"
#include "../forward/BloomPostProcessPass.h"
#include "../SkyboxRenderPass.h"
#include "../../2d/UIPass2D.h"
#include "graphic/Renderer.h"
#include "graphic/Renderer2D.h"
#include "graphic/RenderCommandContext.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IComputePipeline.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/adapters/vulkan/VulkanResources.h"
#include "app/Engine.h"
#include "Logger.h"
#include <glm/glm.hpp>
#include <algorithm>

namespace Prisma::Graphic {

/**
 * @brief 内部渲染目标代理
 * 用于将 ITexture 包装为 IRenderTarget，以便传递给 Pass。
 */
class TextureRenderTargetProxy final : public ITextureRenderTarget {
public:
    TextureRenderTargetProxy(ITexture* texture) : m_texture(texture) {}

    uint32_t GetWidth() const override { return m_texture ? static_cast<uint32_t>(m_texture->GetWidth()) : 0; }
    uint32_t GetHeight() const override { return m_texture ? static_cast<uint32_t>(m_texture->GetHeight()) : 0; }
    TextureFormat GetFormat() const override { return m_texture ? m_texture->GetFormat() : TextureFormat::Unknown; }
    TextureType GetType() const override { return m_texture ? m_texture->GetTextureType() : TextureType::Texture2D; }

    void* GetNativeHandle() const override {
        if (!m_texture) return nullptr;
        auto vkTexture = dynamic_cast<Vulkan::VulkanTexture*>(m_texture);
        if (vkTexture) {
            return reinterpret_cast<void*>(vkTexture->GetVkImageView());
        }
        return nullptr;
    }

    bool IsSwapChain() const override { return false; }
    void Clear(const float color[4]) override {
        if (m_texture) {
            m_texture->Clear(Color(color[0], color[1], color[2], color[3]));
        }
    }

    uint32_t GetMipLevels() const override { return m_texture ? m_texture->GetMipLevels() : 0; }
    uint32_t GetArraySize() const override { return m_texture ? m_texture->GetArraySize() : 0; }
    ITexture* GetTexture() override { return m_texture; }

private:
    ITexture* m_texture;
};

struct ClusterCameraUBO {
    glm::mat4 projection;
    glm::mat4 invProjection;
    glm::mat4 view;
    float nearPlane;
    float farPlane;
    glm::uvec2 gridSize;
    glm::uvec2 screenSize;
    uint32_t totalLights;
    uint32_t numZSlices;
};

ClusteredForwardPipeline::ClusteredForwardPipeline() = default;

ClusteredForwardPipeline::~ClusteredForwardPipeline() {
    Shutdown();
}

int ClusteredForwardPipeline::Initialize(IRenderDevice* device) {
    m_device = device;
    LOG_INFO("ClusteredForwardPipeline", "Initializing Clustered Forward Pipeline...");

    auto* factory = device->GetResourceFactory();
    auto* rm = Engine::Get().GetRenderResourceManager();

    // 1. 加载计算着色器
    auto buildShader = rm->LoadShaderSync("assets/shaders/clustered/cluster_build.comp.spv");
    auto cullShader = rm->LoadShaderSync("assets/shaders/clustered/cluster_cull.comp.spv");

    if (!buildShader || !cullShader) {
        LOG_ERROR("ClusteredForwardPipeline", "Failed to load compute shaders!");
        return -1;
    }

    // 2. 创建计算管线
    m_buildPipeline = factory->CreateComputePipelineImpl();
    m_buildPipeline->SetShader(buildShader);
    if (!m_buildPipeline->Create(device)) return -1;

    m_cullPipeline = factory->CreateComputePipelineImpl();
    m_cullPipeline->SetShader(cullShader);
    if (!m_cullPipeline->Create(device)) return -1;

    // 3. 创建缓冲区
    const uint32_t numClusters = 16 * 9 * 24;
    
    BufferDesc aabbDesc;
    aabbDesc.type = BufferType::Structured;
    aabbDesc.size = numClusters * sizeof(float) * 8;
    aabbDesc.usage = BufferUsage::Default;
    m_clusterAABBs = factory->CreateBufferImpl(aabbDesc);

    BufferDesc lightDesc;
    lightDesc.type = BufferType::Structured;
    lightDesc.size = 1024 * sizeof(Light);
    lightDesc.usage = BufferUsage::Dynamic;
    m_lightBuffer = factory->CreateBufferImpl(lightDesc);

    BufferDesc indexDesc;
    indexDesc.type = BufferType::Structured;
    indexDesc.size = numClusters * 256 * sizeof(uint32_t);
    indexDesc.usage = BufferUsage::Default;
    m_globalLightIndexList = factory->CreateBufferImpl(indexDesc);

    BufferDesc gridDesc;
    gridDesc.type = BufferType::Structured;
    gridDesc.size = numClusters * sizeof(uint32_t) * 2;
    gridDesc.usage = BufferUsage::Default;
    m_clusterGrid = factory->CreateBufferImpl(gridDesc);

    BufferDesc countDesc;
    countDesc.type = BufferType::Structured;
    countDesc.size = sizeof(uint32_t);
    countDesc.usage = BufferUsage::Default;
    m_globalLightCount = factory->CreateBufferImpl(countDesc);

    BufferDesc uboDesc;
    uboDesc.type = BufferType::Constant;
    uboDesc.size = sizeof(ClusterCameraUBO);
    uboDesc.usage = BufferUsage::Dynamic;
    m_cameraUBO = factory->CreateBufferImpl(uboDesc);

    // 4. 初始化基础 Pass
    m_depthPrePass = std::make_shared<DepthPrePass>();
    m_opaquePass = std::make_shared<ClusteredOpaquePass>();
    m_skyboxPass = std::make_shared<SkyboxPass>();
    m_transparentPass = std::make_shared<TransparentPass>();
    m_bloomPass = std::make_shared<BloomPostProcessPass>();
    m_bloomPass->Setup(device);
    m_uiPass = std::make_shared<UIPass2D>();

    // 5. 创建描述符集 (从计算管线获取布局)
    auto buildLayouts = m_buildPipeline->GetDescriptorSetLayouts();
    if (!buildLayouts.empty()) {
        m_buildDS = factory->CreateDescriptorSet(buildLayouts[0].get());
        m_buildDS->BindBuffer(0, m_clusterAABBs.get(), 0, aabbDesc.size, DescriptorType::StorageBuffer);
        m_buildDS->BindBuffer(1, m_cameraUBO.get(), 0, sizeof(ClusterCameraUBO), DescriptorType::UniformBuffer);
        m_buildDS->Update();
    }

    auto cullLayouts = m_cullPipeline->GetDescriptorSetLayouts();
    if (!cullLayouts.empty()) {
        m_cullDS = factory->CreateDescriptorSet(cullLayouts[0].get());
        m_cullDS->BindBuffer(0, m_clusterAABBs.get(), 0, aabbDesc.size, DescriptorType::StorageBuffer);
        m_cullDS->BindBuffer(1, m_lightBuffer.get(), 0, lightDesc.size, DescriptorType::StorageBuffer);
        m_cullDS->BindBuffer(2, m_globalLightIndexList.get(), 0, indexDesc.size, DescriptorType::StorageBuffer);
        m_cullDS->BindBuffer(3, m_clusterGrid.get(), 0, gridDesc.size, DescriptorType::StorageBuffer);
        m_cullDS->BindBuffer(4, m_globalLightCount.get(), 0, sizeof(uint32_t), DescriptorType::StorageBuffer);
        m_cullDS->BindBuffer(5, m_cameraUBO.get(), 0, sizeof(ClusterCameraUBO), DescriptorType::UniformBuffer);
        m_cullDS->Update();
    }

    return 0;
}

void ClusteredForwardPipeline::Shutdown() {
    m_buildPipeline.reset();
    m_cullPipeline.reset();
    m_clusterAABBs.reset();
    m_lightBuffer.reset();
    m_globalLightIndexList.reset();
    m_clusterGrid.reset();
    m_globalLightCount.reset();
    m_cameraUBO.reset();
    m_buildDS.reset();
    m_cullDS.reset();

    m_depthPrePass.reset();
    m_opaquePass.reset();
    m_skyboxPass.reset();
    m_transparentPass.reset();
    if (m_bloomPass) {
        m_bloomPass->Cleanup();
        m_bloomPass.reset();
    }
    m_device = nullptr;
}

void ClusteredForwardPipeline::Execute(const RenderContext& ctx) {
    if (!m_device || !ctx.commandBuffer) return;

    static bool s_FirstFrameLogged = false;
    bool shouldLog = !s_FirstFrameLogged;
    if (shouldLog) {
        LOG_INFO("ClusteredForwardPipeline", "=== First Frame Detailed Log ===");
        LOG_INFO("ClusteredForwardPipeline", "Context: Res={}x{}, Lights={}, Frame={}", ctx.width, ctx.height, ctx.lights.size(), ctx.frameIndex);
        LOG_INFO("ClusteredForwardPipeline", "ClearColor: [{}, {}, {}, {}]", ctx.clearColor.x, ctx.clearColor.y, ctx.clearColor.z, ctx.clearColor.w);
        s_FirstFrameLogged = true;
    }

    auto cmd = ctx.commandBuffer;

    // 1. 更新 Camera UBO
    ClusterCameraUBO ubo{};
    ubo.projection = ctx.camera.projectionMatrix;
    ubo.invProjection = glm::inverse(ctx.camera.projectionMatrix);
    ubo.view = ctx.camera.viewMatrix;
    ubo.nearPlane = ctx.camera.nearPlane;
    ubo.farPlane = ctx.camera.farPlane;
    ubo.gridSize = glm::uvec2(16, 9);
    ubo.screenSize = glm::uvec2(ctx.width, ctx.height);
    ubo.totalLights = (uint32_t)ctx.lights.size();
    ubo.numZSlices = 24;
    m_cameraUBO->UpdateData(&ubo, sizeof(ubo), 0);

    // 2. 更新 Light Buffer
    if (!ctx.lights.empty()) {
        m_lightBuffer->UpdateData(ctx.lights.data(), (uint32_t)(ctx.lights.size() * sizeof(Light)), 0);
    }

    // 3. 重置 Global Light Count
    uint32_t zero = 0;
    m_globalLightCount->UpdateData(&zero, sizeof(uint32_t), 0);

    // 4. Dispatch Build Clusters
    cmd->SetComputePipeline(m_buildPipeline.get());
    cmd->BindDescriptorSet(0, m_buildDS.get());
    cmd->Dispatch(16, 9, 24);
    if (shouldLog) LOG_INFO("ClusteredForwardPipeline", "Dispatched Cluster Build (16x9x24)");

    // 5. Dispatch Light Culling
    cmd->SetComputePipeline(m_cullPipeline.get());
    cmd->BindDescriptorSet(0, m_cullDS.get());
    cmd->Dispatch(16, 9, 24);
    if (shouldLog) LOG_INFO("ClusteredForwardPipeline", "Dispatched Light Culling (16x9x24)");

    // 5.1 Pipeline Barrier (Compute Write -> Fragment Read)
    cmd->PipelineBarrier();

    // 6. 开始渲染 Pass
    m_device->BeginSwapChainRenderPass(ctx.clearColor);

    // 7. 执行 Opaque Pass
    cmd->SetViewport(Viewport{0.0f, 0.0f, (float)ctx.width, (float)ctx.height, 0.0f, 1.0f});
    cmd->SetScissorRect(Rect{0, 0, (int)ctx.width, (int)ctx.height});

    m_opaquePass->SetViewMatrix(ctx.camera.viewMatrix);
    m_opaquePass->SetProjectionMatrix(ctx.camera.projectionMatrix);
    m_opaquePass->SetClusteredResources(m_lightBuffer.get(), m_globalLightIndexList.get(), m_clusterGrid.get(), m_cameraUBO.get());

    const auto& commands = Renderer::GetCommandQueue();
    size_t initialCommandCount = commands.size(); // 记录当前 3D 命令数量

    if (shouldLog) {
        LOG_INFO("ClusteredForwardPipeline", "Render Queue Size (3D): {}", initialCommandCount);
        for (size_t i = 0; i < commands.size(); ++i) {
             LOG_INFO("ClusteredForwardPipeline", "  Command {}: Mesh={}", i, commands[i].mesh ? "Valid" : "Null");
        }
    }

    if (initialCommandCount > 0) {
        m_opaquePass->Execute(cmd, commands, m_device);
    }

    // ── Skybox + Transparent Passes (inside render pass) ──
    {
        PrismaMath::mat4 view = ctx.camera.viewMatrix;
        PrismaMath::mat4 proj = ctx.camera.projectionMatrix;

        SceneData sceneData;
        sceneData.camera.view = view;
        sceneData.camera.projection = proj;
        sceneData.camera.viewProjection = proj * view;
        sceneData.camera.position = ctx.camera.position;
        sceneData.camera.nearPlane = ctx.camera.nearPlane;
        sceneData.camera.farPlane = ctx.camera.farPlane;
        sceneData.time.ts = ctx.deltaTime;
        sceneData.viewport.width = ctx.width;
        sceneData.viewport.height = ctx.height;

        RenderCommandContext fallbackContext;
        IDeviceContext* deviceContext = &fallbackContext;

        PassExecutionContext passContext;
        passContext.deviceContext = deviceContext;
        passContext.sceneData = &sceneData;
        passContext.renderTarget = nullptr; // swap chain rendering

        if (m_skyboxPass) {
            m_skyboxPass->SetViewMatrix(view);
            m_skyboxPass->SetProjectionMatrix(proj);
            m_skyboxPass->Execute(passContext);
        }

        if (m_transparentPass) {
            m_transparentPass->SetViewMatrix(view);
            m_transparentPass->SetProjectionMatrix(proj);
            m_transparentPass->Execute(passContext);
        }
    }

    // 8. 渲染 Overlay (UI 会向队列添加新命令)
    RenderOverlay(ctx);
    
    const auto& allCommands = Renderer::GetCommandQueue();
    const auto& gizmoCommands = Renderer::GetGizmoQueue();
    if (shouldLog) LOG_INFO("ClusteredForwardPipeline", "Render Queue Size: Total={}, Gizmo={}", allCommands.size(), gizmoCommands.size());

    // 绘制所有新添加的命令 (UI)
    if (allCommands.size() > initialCommandCount) {
        std::vector<RenderCommand> uiOnlyCommands;
        uiOnlyCommands.assign(allCommands.begin() + initialCommandCount, allCommands.end());

        // UI 使用正交投影 (Top-Down)
        PrismaMath::mat4 uiProj = glm::orthoLH_ZO(0.0f, (float)ctx.width, (float)ctx.height, 0.0f, -1.0f, 1.0f);
        m_opaquePass->SetViewMatrix(PrismaMath::mat4(1.0f));
        m_opaquePass->SetProjectionMatrix(uiProj);

        // 暂时使用 OpaquePass 绘制，由于 UI 坐标已经是像素坐标，
        // 且 command.transform 包含 Renderer2D 计算好的位置，
        // 使用 uiProj 应该能正确显示在屏幕上。
        m_opaquePass->Execute(cmd, uiOnlyCommands, m_device);
    }

    // 绘制 Gizmos
    if (!gizmoCommands.empty()) {
        // Gizmos 也暂时使用 UI 投影或者 3D 投影？
        // Renderer2D::FlushGizmo 提交的坐标也是像素坐标（在 StatsOverlay 中）
        PrismaMath::mat4 uiProj = glm::orthoLH_ZO(0.0f, (float)ctx.width, (float)ctx.height, 0.0f, -1.0f, 1.0f);
        m_opaquePass->SetViewMatrix(PrismaMath::mat4(1.0f));
        m_opaquePass->SetProjectionMatrix(uiProj);
        m_opaquePass->Execute(cmd, gizmoCommands, m_device);
    }

    // ── Bloom Post-Process (offscreen only, requires targetTexture) ──
    if (m_bloomPass && m_bloomPass->IsReady() && ctx.commandBuffer && ctx.targetTexture) {
        TextureRenderTargetProxy bloomTarget(ctx.targetTexture);
        m_bloomPass->Execute(ctx.commandBuffer, ctx.targetTexture, &bloomTarget);
    }

    // 9. 结束渲染 Pass
    m_device->EndSwapChainRenderPass();
    
    Renderer::ClearQueue();
    if (shouldLog) LOG_INFO("ClusteredForwardPipeline", "=== End First Frame Log ===");
}

void ClusteredForwardPipeline::RenderOverlay(const RenderContext& ctx) {
    auto cmd = ctx.commandBuffer;
    if (m_uiPass && cmd) {
        m_uiPass->RenderUI(cmd, m_device, ctx.width, ctx.height);
    }
    Renderer::ClearGizmoQueue();
}

} // namespace Prisma::Graphic
