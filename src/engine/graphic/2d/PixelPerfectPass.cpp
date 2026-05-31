#include "PixelPerfectPass.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/ISwapChain.h"
#include "graphic/RenderDesc.h"
#include "Logger.h"
#include <algorithm>
#include <glm/glm.hpp>

namespace Prisma::Graphic {

PixelPerfectPass::PixelPerfectPass() : LogicalPass("PixelPerfect") {
    m_priority = 150; // After Canvas (~130), before PostProcess (~200), before UI (~250)
}

void PixelPerfectPass::Initialize(IRenderDevice* device) {
    EnsureResources(device);
}

void PixelPerfectPass::Shutdown() {
    m_offscreenTexture.reset();
    m_depthTexture.reset();
    m_offscreenRT.reset();
    m_blitPSO.reset();
    m_vertShader.reset();
    m_fragShader.reset();
    m_pointSampler.reset();
    m_blitDescSetLayout.reset();
    m_blitDescSet.reset();
}

void PixelPerfectPass::EnsureResources(IRenderDevice* device) {
    if (m_offscreenRT) return;
    if (!device) return;

    auto* fac = device->GetResourceFactory();
    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!fac || !rm) return;

    // ── 1. 创建离屏颜色纹理 (256×224, RGBA8_UNorm) ──
    TextureDesc texDesc;
    texDesc.type = TextureType::Texture2D;
    texDesc.format = TextureFormat::BGRA8_UNorm;  // 对齐交换链格式，避免 RenderPass 兼容性错误
    texDesc.width = m_logicW;
    texDesc.height = m_logicH;
    texDesc.allowRenderTarget = true;
    texDesc.allowShaderResource = true;
    texDesc.mipLevels = 1;

    m_offscreenTexture = fac->CreateTextureImpl(texDesc);
    if (!m_offscreenTexture) {
        LOG_ERROR("PixelPerfect", "Failed to create offscreen texture ({}x{})", m_logicW, m_logicH);
        return;
    }
    LOG_INFO("PixelPerfect", "Offscreen texture created: {}x{}", m_logicW, m_logicH);

    // ── 2. 创建离屏深度纹理（D32_Float，匹配交换链深度格式） ──
    TextureDesc depthDesc;
    depthDesc.type = TextureType::Texture2D;
    depthDesc.format = TextureFormat::D32_Float;
    depthDesc.width = m_logicW;
    depthDesc.height = m_logicH;
    depthDesc.allowRenderTarget = true;
    depthDesc.allowDepthStencil = true;
    depthDesc.allowShaderResource = false;
    depthDesc.mipLevels = 1;

    m_depthTexture = fac->CreateTextureImpl(depthDesc);
    if (!m_depthTexture) {
        LOG_WARN("PixelPerfect", "Failed to create offscreen depth texture; 2D passes requiring depth may not work");
    }

    // ── 3. 包装为 RT 代理 ──
    m_offscreenRT = std::make_shared<OffscreenRTProxy>(m_offscreenTexture);

    // ── 4. 创建点采样器（最近邻） ──
    SamplerDesc samplerDesc;
    samplerDesc.filter = TextureFilter::Point;
    samplerDesc.addressU = TextureAddressMode::Clamp;
    samplerDesc.addressV = TextureAddressMode::Clamp;
    samplerDesc.addressW = TextureAddressMode::Clamp;
    m_pointSampler = rm->CreateSampler(samplerDesc);

    // ── 4. 加载并编译 Blit 着色器 ──
    m_vertShader = rm->LoadShaderSync("assets/shaders/FullscreenTri.vert.spv", "main");
    m_fragShader = rm->LoadShaderSync("assets/shaders/PixelPerfectBlit.frag.spv", "main");

    if (!m_vertShader || !m_fragShader) {
        LOG_ERROR("PixelPerfect", "Failed to load blit shaders");
        return;
    }

    // ── 5. 创建 Blit 管线状态 ──
    auto pso = fac->CreatePipelineStateImpl();
    if (!pso) {
        LOG_ERROR("PixelPerfect", "Failed to create PSO impl");
        return;
    }

    pso->SetShader(ShaderType::Vertex, m_vertShader);
    pso->SetShader(ShaderType::Pixel, m_fragShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);

    // 关闭深度测试
    DepthStencilState ds;
    ds.depthEnable = false;
    ds.depthWriteEnable = false;
    pso->SetDepthStencilState(ds);

    // 无裁剪（全屏三角形）
    RasterizerState rs;
    rs.cullEnable = false;
    pso->SetRasterizerState(rs);

    // 关闭混合（直接覆盖）
    BlendState bs;
    bs.blendEnable = false;
    pso->SetBlendState(bs);

    // 无顶点输入（使用 vertex_id 生成位置和 UV）
    pso->SetInputLayout({});

    // 设置渲染目标格式为交换链格式 (RGBA8_UNorm)
    pso->SetRenderTargetFormat(0, TextureFormat::BGRA8_UNorm);  // 对齐交换链格式 (B8G8R8A8)

    if (pso->Create(device)) {
        m_blitPSO = std::shared_ptr<IPipelineState>(std::move(pso));
        LOG_INFO("PixelPerfect", "Blit PSO created");
    } else {
        LOG_ERROR("PixelPerfect", "Failed to create blit PSO: {}", pso->GetErrors());
    }

    // ── 6. 创建描述符集布局和描述符集 ──
    // Blit shader 使用:
    //   binding 0: texture2D (sampled image)
    //   binding 1: sampler
    if (m_blitPSO && m_offscreenTexture && m_pointSampler) {
        auto& layouts = m_blitPSO->GetDescriptorSetLayouts();
        if (!layouts.empty()) {
            m_blitDescSetLayout = layouts[0];
            m_blitDescSet = fac->CreateDescriptorSet(m_blitDescSetLayout.get());
            if (m_blitDescSet) {
                m_blitDescSet->BindTexture(0, m_offscreenTexture.get(), m_pointSampler.get());
                m_blitDescSet->Update();
                LOG_INFO("PixelPerfect", "Blit descriptor set created");
            }
        }
    }
}

void PixelPerfectPass::CalculateViewport(uint32_t windowW, uint32_t windowH,
    int& outX, int& outY, uint32_t& outW, uint32_t& outH, uint32_t& scale) {
    uint32_t scaleX = windowW / m_logicW;
    uint32_t scaleY = windowH / m_logicH;
    scale = std::min(scaleX, scaleY);
    if (scale < 1) scale = 1;

    outW = m_logicW * scale;
    outH = m_logicH * scale;
    outX = (static_cast<int>(windowW) - static_cast<int>(outW)) / 2;
    outY = (static_cast<int>(windowH) - static_cast<int>(outH)) / 2;
}

void PixelPerfectPass::Execute(const PassExecutionContext& /*context*/) {
    // PixelPerfectPass 不通过标准的 PassExecutionContext 执行。
    // 请使用 BlitToSwapChain() 在 Pipeline2D::Execute() 中显式调用。
}

void PixelPerfectPass::BlitToSwapChain(ICommandBuffer* cmd,
                                       IRenderDevice* device,
                                       IRenderTarget* /*swapChainTarget*/) {
    if (!cmd || !device || !m_offscreenTexture || !m_blitPSO) return;

    // 获取交换链尺寸
    auto* swapChain = device->GetSwapChain();
    uint32_t winW = swapChain ? swapChain->GetWidth() : (m_logicW * 3);
    uint32_t winH = swapChain ? swapChain->GetHeight() : (m_logicH * 3);

    // 计算整数缩放的视口（居中，保留像素完整性）
    int vpX, vpY;
    uint32_t vpW, vpH, scale;
    CalculateViewport(winW, winH, vpX, vpY, vpW, vpH, scale);

    // 设置视口
    Viewport vp;
    vp.x = static_cast<float>(vpX);
    vp.y = static_cast<float>(vpY);
    vp.width = static_cast<float>(vpW);
    vp.height = static_cast<float>(vpH);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    cmd->SetViewport(vp);

    // 设置裁剪矩形
    Rect scissor;
    scissor.x = vpX;
    scissor.y = vpY;
    scissor.width = static_cast<int>(vpW);
    scissor.height = static_cast<int>(vpH);
    cmd->SetScissorRect(scissor);

    // 绑定 Blit 管线
    cmd->SetPipelineState(m_blitPSO.get());

    // 绑定离屏纹理作为着色器资源
    if (m_blitDescSet) {
        cmd->BindDescriptorSet(0, m_blitDescSet.get());
    }

    // 全屏三角形绘制（3 个顶点，无 VBO 开销）
    cmd->Draw(3, 1, 0);

    static bool s_firstFrame = true;
    if (s_firstFrame) {
        LOG_INFO("PixelPerfect", "Blit: {}x{} -> {}x{} (scale={}, offset={},{})",
                 m_logicW, m_logicH, vpW, vpH, scale, vpX, vpY);
        s_firstFrame = false;
    }
}

} // namespace Prisma::Graphic
