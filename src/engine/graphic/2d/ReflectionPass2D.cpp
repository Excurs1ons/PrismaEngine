#include "ReflectionPass2D.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/RenderCommandContext.h"

// Vulkan-specific includes for swapchain image access (CaptureScene)
#include "adapters/vulkan/RenderDeviceVulkan.h"
#include "adapters/vulkan/VulkanSwapChain.h"
#include "adapters/vulkan/VulkanResources.h"
#include "Logger.h"

namespace Prisma::Graphic {

ReflectionPass2D::ReflectionPass2D()
    : ForwardRenderPass("ReflectionPass2D") {
    m_priority = 90; // After OpaquePass (~80), before SkyboxPass (~100)
}

ReflectionPass2D::~ReflectionPass2D() {}

void ReflectionPass2D::Update(Prisma::Timestep ts) {
    UpdateTime(ts);
}

void ReflectionPass2D::Execute(const PassExecutionContext& context) {
    // Context-based execution (unused in v1, kept for interface compliance)
}

// ────────────────────────────────────────────────────────────────────────────
// EnsureResources — 创建反射源纹理、着色器、PSO、UBO、描述符集
// ────────────────────────────────────────────────────────────────────────────
void ReflectionPass2D::EnsureResources(uint32_t width, uint32_t height, IRenderDevice* device) {
    if (m_width == width && m_height == height && m_reflectionSource && m_reflectionUBO) return;

    m_width = width;
    m_height = height;

    auto rf = device->GetResourceFactory();
    auto rm = Engine::Get().GetRenderResourceManager();
    if (!rf || !rm) return;

    // 1. Create ReflectionSource texture (RGBA8, viewport-sized, blit destination + shader resource)
    bool needNewTex = !m_reflectionSource;
    if (m_reflectionSource) {
        float tw = m_reflectionSource->GetWidth();
        float th = m_reflectionSource->GetHeight();
        needNewTex = (static_cast<uint32_t>(tw) != width || static_cast<uint32_t>(th) != height);
    }
    if (needNewTex) {
        TextureDesc desc;
        desc.type = TextureType::Texture2D;
        desc.width = width;
        desc.height = height;
        desc.format = TextureFormat::RGBA8_UNorm;
        desc.mipLevels = 1;
        desc.allowRenderTarget = false;     // Not used as RT, only blit destination
        desc.allowShaderResource = true;    // Readable in shaders
        auto tex = rf->CreateTextureImpl(desc);
        if (tex) {
            m_reflectionSource = std::move(tex);
            LOG_INFO("ReflectionPass2D", "已创建 ReflectionSource 纹理 ({}x{})", width, height);
        }
    }

    // 2. Load shaders
    if (!m_vertexShader) {
        m_vertexShader = rm->LoadShaderSync("assets/shaders/Renderer2D.vert.spv", "main");
        if (!m_vertexShader) {
            LOG_WARNING("ReflectionPass2D", "无法加载 Renderer2D.vert.spv, 尝试使用内置默认着色器");
            m_vertexShader = rm->LoadShaderSync("Default");
        }
    }

    if (!m_fragmentShader) {
        m_fragmentShader = rm->LoadShaderSync("assets/shaders/ReflectionSprite.frag.spv", "main");
        if (!m_fragmentShader) {
            LOG_WARNING("ReflectionPass2D", "无法加载 ReflectionSprite.frag.spv — 反射着色器未就绪 (需要先编译 SPIR-V)");
            // 不回退到默认着色器 — 反射效果不生效时降级为无反射
        }
    }

    // 3. Create PSO (仅当着色器都可用时)
    if (!m_reflectionPSO && m_vertexShader && m_fragmentShader) {
        auto pso = rf->CreatePipelineStateImpl();
        if (pso) {
            pso->SetShader(ShaderType::Vertex, m_vertexShader);
            pso->SetShader(ShaderType::Pixel, m_fragmentShader);
            pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);

            // Blend: alpha blending for semi-transparent reflections
            BlendState blendState;
            blendState.blendEnable = true;
            blendState.srcBlend = BlendFactorType::SrcAlpha;
            blendState.destBlend = BlendFactorType::InvSrcAlpha;
            blendState.blendOp = BlendOp::Add;
            blendState.srcBlendAlpha = BlendFactorType::One;
            blendState.destBlendAlpha = BlendFactorType::Zero;
            blendState.blendOpAlpha = BlendOp::Add;
            pso->SetBlendState(blendState);

            // Depth: test ON, write OFF (reflections overlay scene)
            DepthStencilState dsState;
            dsState.depthEnable = true;
            dsState.depthWriteEnable = false;
            dsState.depthFunc = ComparisonFunc::LessEqual;
            pso->SetDepthStencilState(dsState);

            // Rasterizer: no culling for 2D
            RasterizerState rsState;
            rsState.cullMode = CullMode::None;
            rsState.fillMode = FillMode::Solid;
            pso->SetRasterizerState(rsState);

            // Render target formats (match swapchain)
            pso->SetRenderTargetFormats({TextureFormat::RGBA8_UNorm});
            pso->SetDepthStencilFormat(TextureFormat::D32_Float);

            if (pso->Create(device)) {
                m_reflectionPSO = std::shared_ptr<IPipelineState>(std::move(pso));
                LOG_INFO("ReflectionPass2D", "已创建反射 PSO");

                // Create descriptor set layout from explicit resource bindings
                // (PSO creation already built the pipeline layout; this matching layout
                //  is used for creating descriptor sets at runtime)
                std::vector<ShaderResource> resources = {
                    {"AlbedoMap",       ShaderResource::Type::Sampler2D,     0, 0, 1, 0},
                    {"ReflectionSource",ShaderResource::Type::Sampler2D,     0, 1, 1, 0},
                    {"ReflectionUBO",   ShaderResource::Type::UniformBuffer, 0, 3, 1,
                     static_cast<uint32_t>(sizeof(ReflectionUBOData))}
                };
                m_reflectionDSLayout = rf->CreateDescriptorSetLayout(resources);
                if (m_reflectionDSLayout) {
                    m_reflectionDS = rf->CreateDescriptorSet(m_reflectionDSLayout.get());
                    if (m_reflectionDS) {
                        LOG_INFO("ReflectionPass2D", "已创建反射描述符集");
                    }
                }
            } else {
                LOG_ERROR("ReflectionPass2D", "创建反射 PSO 失败");
            }
        }
    }

    // 4. Create UBO buffer for reflection params
    if (!m_reflectionUBO) {
        BufferDesc ubd;
        ubd.type = BufferType::Constant;
        ubd.size = sizeof(ReflectionUBOData);
        ubd.usage = BufferUsage::Dynamic;
        ubd.initialData = &m_uboData;
        auto buf = rf->CreateBufferImpl(ubd);
        if (buf) {
            m_reflectionUBO = std::move(buf);
            LOG_INFO("ReflectionPass2D", "已创建反射 UBO ({} bytes)", sizeof(ReflectionUBOData));
        }
    }
}

// ────────────────────────────────────────────────────────────────────────────
// ExecuteReflection — 绘制反射表面 (v1: 全屏三角形占位)
// ────────────────────────────────────────────────────────────────────────────
void ReflectionPass2D::ExecuteReflection(ICommandBuffer* cmd, IRenderDevice* device,
                                          uint32_t width, uint32_t height) {
    if (!cmd || !device) return;

    EnsureResources(width, height, device);
    if (!m_reflectionPSO || !m_reflectionUBO) {
        LOG_DEBUG("ReflectionPass2D", "资源未就绪 (缺少 PSO 或 UBO)，跳过反射渲染");
        return;
    }

    // Update UBO with current reflection parameters
    m_reflectionUBO->UpdateData(&m_uboData, sizeof(ReflectionUBOData), 0);

    // Set pipeline state
    cmd->SetPipelineState(m_reflectionPSO.get());
    cmd->SetViewport(Viewport{0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f});
    cmd->SetScissorRect(Rect{0, 0, (int)width, (int)height});

    // ── v1: 反射绘制是占位 ──
    // 完整实现需要:
    //   1. 从场景收集反射表面 (reflective sprites/quads)
    //   2. 绑定描述符集 (AlbedoMap @b0, ReflectionSource @b1, UBO @b3)
    //   3. 对每个反射表面设置 push constants (MVP + Color) 并绘制
    //
    // v1 暂不绘制 — 反射源纹理可通过 GetReflectionSource() 获取，
    // 由应用层使用标准 Renderer2D 批处理系统进行绘制。

    LOG_DEBUG("ReflectionPass2D", "ExecuteReflection: {}x{} (PSO 已就绪)", width, height);
}

// ────────────────────────────────────────────────────────────────────────────
// CaptureScene — 将当前帧的 swapchain 颜色拷贝到 ReflectionSource 纹理
// ────────────────────────────────────────────────────────────────────────────
void ReflectionPass2D::CaptureScene(ICommandBuffer* cmd, IRenderDevice* device,
                                     uint32_t width, uint32_t height) {
    if (!cmd || !device) return;

    EnsureResources(width, height, device);
    if (!m_reflectionSource) return;

    // v1: 使用 ITexture::CopyFrom 进行简单拷贝
    // (需要后端 VulkanTexture 支持正确的布局转换)
    //
    // 完整 Vulkan 拷贝序列 (vkCmdCopyImage):
    //   1. 通过 RenderDeviceVulkan 获取 swapchain 的当前 VkImage
    //   2. 插入屏障: COLOR_ATTACHMENT_OPTIMAL → TRANSFER_SRC_OPTIMAL
    //   3. 通过 VulkanTexture 获取反射源纹理的 VkImage
    //   4. 插入屏障: SHADER_READ_ONLY_OPTIMAL → TRANSFER_DST_OPTIMAL
    //   5. vkCmdCopyImage(swapchainImage, ..., reflectionImage, ..., region)
    //   6. 转换回原始布局
    //
    // 当前简化实现: 通过 ISwapChain/ITexture 接口尝试拷贝 (可能不支持所有后端)

    auto* vkDev = dynamic_cast<Vulkan::RenderDeviceVulkan*>(device);
    if (!vkDev) {
        LOG_DEBUG("ReflectionPass2D", "非 Vulkan 后端，跳过场景捕获");
        return;
    }

    // 方法1: 通过 swapchain 的 rendertarget 进行拷贝
    auto* swapChain = vkDev->GetSwapChain();
    if (!swapChain) return;

    auto* currentRT = swapChain->GetCurrentRenderTarget();
    if (currentRT) {
        m_reflectionSource->CopyFrom(currentRT, 0, 0, 0, 0);
        LOG_DEBUG("ReflectionPass2D", "场景捕获 (CopyFrom): {}x{}", width, height);
    } else {
        // 方法2: 通过 CaptureFrame 回读再上传 (慢路径)
        LOG_DEBUG("ReflectionPass2D", "场景捕获: 无法获取 swapchain RT ({}x{})", width, height);
    }
}

} // namespace Prisma::Graphic
