#include "PostProcessPass2D.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/ISampler.h"
#include "graphic/interfaces/IBuffer.h"
#include "Logger.h"

namespace Prisma::Graphic {

PostProcessPass2D::PostProcessPass2D()
    : ForwardRenderPass("PostProcessPass2D") {
    m_priority = 200; // 在所有渲染完成后执行
}

PostProcessPass2D::~PostProcessPass2D() {}

void PostProcessPass2D::Update(Prisma::Timestep ts) {
    UpdateTime(ts);
}

void PostProcessPass2D::Execute(const PassExecutionContext& context) {
    LOG_DEBUG("PostProcess2D", "Execute: rt={} (use Process() instead)", 
              (void*)context.renderTarget);
}

void PostProcessPass2D::EnsureResources(uint32_t width, uint32_t height, IRenderDevice* device) {
    if (m_width == width && m_height == height && m_pso) return;

    m_width = width;
    m_height = height;

    auto rf = device->GetResourceFactory();
    auto rm = Engine::Get().GetRenderResourceManager();
    if (!rf || !rm) return;

    // 加载全屏三角形顶点着色器（所有效果共享）
    if (!m_vertShader) {
        m_vertShader = rm->LoadShaderSync("assets/shaders/FullscreenTri.vert.spv", "main");
    }
    if (!m_fragShader) {
        m_fragShader = rm->LoadShaderSync("assets/shaders/PostProcess2D.frag.spv", "main");
    }

    if (!m_pso && m_vertShader && m_fragShader) {
        auto pso = rf->CreatePipelineStateImpl();
        pso->SetShader(ShaderType::Vertex, m_vertShader);
        pso->SetShader(ShaderType::Pixel, m_fragShader);
        pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
        
        // 关闭深度测试
        DepthStencilState ds;
        ds.depthEnable = false;
        ds.depthWriteEnable = false;
        pso->SetDepthStencilState(ds);

        if (pso->Create(device)) {
            m_pso = std::shared_ptr<IPipelineState>(std::move(pso));
        }
    }
}

void PostProcessPass2D::EnsureCRTResources(IRenderDevice* device) {
    if (m_crtPSO) return;
    if (!device) return;

    auto* rf = device->GetResourceFactory();
    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rf || !rm) return;

    // ── 1. 加载 CRT 着色器 ──
    m_crtVertShader = rm->LoadShaderSync("assets/shaders/FullscreenTri.vert.spv", "main");
    m_crtFragShader = rm->LoadShaderSync("assets/shaders/CRTScanline.frag.spv", "main");
    if (!m_crtVertShader || !m_crtFragShader) {
        LOG_ERROR("PostProcess2D", "Failed to load CRT shaders");
        return;
    }

    // ── 2. 创建 CRT PSO ──
    auto pso = rf->CreatePipelineStateImpl();
    if (!pso) {
        LOG_ERROR("PostProcess2D", "Failed to create CRT PSO impl");
        return;
    }
    pso->SetShader(ShaderType::Vertex, m_crtVertShader);
    pso->SetShader(ShaderType::Pixel, m_crtFragShader);
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

    // 无顶点输入（gl_VertexIndex 生成位置和 UV）
    pso->SetInputLayout({});

    // 渲染目标格式为交换链格式
    pso->SetRenderTargetFormat(0, TextureFormat::RGBA8_UNorm);

    if (!pso->Create(device)) {
        LOG_ERROR("PostProcess2D", "Failed to create CRT PSO: {}", pso->GetErrors());
        return;
    }
    m_crtPSO = std::shared_ptr<IPipelineState>(std::move(pso));
    LOG_INFO("PostProcess2D", "CRT PSO created");

    // ── 3. 创建点采样器（最近邻，匹配像素完美离屏纹理） ──
    SamplerDesc samplerDesc;
    samplerDesc.filter = TextureFilter::Point;
    samplerDesc.addressU = TextureAddressMode::Clamp;
    samplerDesc.addressV = TextureAddressMode::Clamp;
    samplerDesc.addressW = TextureAddressMode::Clamp;
    m_crtSampler = rm->CreateSampler(samplerDesc);

    // ── 4. 创建描述符集用于绑定离屏纹理 ──
    if (m_crtPSO) {
        auto& layouts = m_crtPSO->GetDescriptorSetLayouts();
        if (!layouts.empty()) {
            m_crtDescSetLayout = layouts[0];
            m_crtDescSet = rf->CreateDescriptorSet(m_crtDescSetLayout.get());
            LOG_INFO("PostProcess2D", "CRT descriptor set created");
        }
    }
}

void PostProcessPass2D::Process(ICommandBuffer* cmd, IRenderDevice* device,
                                ITexture* input, IRenderTarget* /*output*/) {
    if (!cmd || !device || !input) return;

    bool hasGrayscale = m_effects[static_cast<int>(EffectType::Grayscale)];
    bool hasBloom     = m_effects[static_cast<int>(EffectType::Bloom)];
    bool hasDistortion = m_effects[static_cast<int>(EffectType::Distortion)];
    bool hasCRT       = m_effects[static_cast<int>(EffectType::CRT)];

    if (!hasGrayscale && !hasBloom && !hasDistortion && !hasCRT) return;

    uint32_t w = static_cast<uint32_t>(input->GetWidth());
    uint32_t h = static_cast<uint32_t>(input->GetHeight());

    // 确保基础资源（顶点着色器 FullscreenTri.vert）
    EnsureResources(w, h, device);

    // ═══════════════════════════════════════════════════════════
    // 处理顺序: Grayscale → Bloom → Distortion → CRT
    // 每个效果读取 input 纹理，写入当前活跃 RT
    // ═══════════════════════════════════════════════════════════

    // ── 1. Grayscale ──
    if (hasGrayscale) {
        EnsureGrayscaleResources(device);
        if (m_grayscalePSO && m_grayscaleDescSet && m_grayscaleSampler) {
            m_grayscaleDescSet->BindTexture(0, input, m_grayscaleSampler.get());
            m_grayscaleDescSet->Update();

            cmd->SetPipelineState(m_grayscalePSO.get());
            cmd->BindDescriptorSet(0, m_grayscaleDescSet.get());
            cmd->PushConstants(ShaderType::VertexAndPixel, &m_grayscaleIntensity, sizeof(float));
            cmd->Draw(3, 1, 0);
        }
    }

    // ── 2. Bloom（单通道 5×5 Gaussian + 合成） ──
    if (hasBloom) {
        float iw = input->GetWidth();
        float ih = input->GetHeight();
        uint32_t bw = std::max(1u, static_cast<uint32_t>(iw / 4));
        uint32_t bh = std::max(1u, static_cast<uint32_t>(ih / 4));
        EnsureBloomResources(device, bw, bh);

        if (m_bloomPSO && m_bloomDescSet && m_bloomSampler) {
            m_bloomDescSet->BindTexture(0, input, m_bloomSampler.get());
            m_bloomDescSet->Update();

            cmd->SetPipelineState(m_bloomPSO.get());
            cmd->BindDescriptorSet(0, m_bloomDescSet.get());
            cmd->PushConstants(ShaderType::VertexAndPixel, &m_bloomIntensity, sizeof(float));
            cmd->Draw(3, 1, 0);
        }
    }

    // ── 3. Distortion ──
    if (hasDistortion) {
        EnsureDistortionResources(device);
        if (m_distortionPSO && m_distortionDescSet && m_distortionSampler) {
            m_distortionDescSet->BindTexture(0, input, m_distortionSampler.get());
            m_distortionDescSet->Update();

            struct DistParams {
                float time, speed, amount, warp;
            };
            DistParams dp = {};
            dp.time   = m_totalTime;
            dp.speed  = m_distortionSpeed;
            dp.amount = m_distortionAmount;
            dp.warp   = 10.0f;

            cmd->SetPipelineState(m_distortionPSO.get());
            cmd->BindDescriptorSet(0, m_distortionDescSet.get());
            cmd->PushConstants(ShaderType::VertexAndPixel, &dp, sizeof(dp));
            cmd->Draw(3, 1, 0);
        }
    }

    // ── 4. CRT（保留原有实现，仅将早期返回改为条件分支） ──
    if (hasCRT) {
        EnsureCRTResources(device);
        if (m_crtPSO && m_crtDescSet && m_crtSampler) {
            m_crtDescSet->BindTexture(0, input, m_crtSampler.get());
            m_crtDescSet->Update();

            cmd->SetPipelineState(m_crtPSO.get());
            cmd->BindDescriptorSet(0, m_crtDescSet.get());

            struct {
                float scanlineIntensity;
                float chromaticAberration;
                float brightness;
                float contrast;
                float logicalWidth;
                float logicalHeight;
            } params = {};

            params.scanlineIntensity    = m_crtScanlineIntensity;
            params.chromaticAberration  = m_crtChromaticAberration;
            params.brightness           = m_crtBrightness;
            params.contrast             = m_crtContrast;
            params.logicalWidth         = static_cast<float>(input->GetWidth());
            params.logicalHeight        = static_cast<float>(input->GetHeight());

            cmd->PushConstants(ShaderType::VertexAndPixel, &params, sizeof(params));
            cmd->Draw(3, 1, 0);

            LOG_TRACE("PostProcess2D", "CRT processed: {}x{} (scanline={}, chroma={})",
                      input->GetWidth(), input->GetHeight(),
                      m_crtScanlineIntensity, m_crtChromaticAberration);
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// Grayscale 资源创建
// ═══════════════════════════════════════════════════════════════
void PostProcessPass2D::EnsureGrayscaleResources(IRenderDevice* device) {
    if (m_grayscalePSO) return;
    if (!device || !m_vertShader) return;

    auto* rf = device->GetResourceFactory();
    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rf || !rm) return;

    // 1. 加载 Grayscale 片段着色器
    m_grayscaleFragShader = rm->LoadShaderSync("assets/shaders/2d/Grayscale.frag.spv", "main");
    if (!m_grayscaleFragShader) {
        LOG_ERROR("PostProcess2D", "Failed to load Grayscale shader");
        return;
    }

    // 2. 创建 PSO（与 CRT 相同的全屏三角形设置）
    auto pso = rf->CreatePipelineStateImpl();
    if (!pso) { LOG_ERROR("PostProcess2D", "Failed to create Grayscale PSO impl"); return; }

    pso->SetShader(ShaderType::Vertex, m_vertShader);
    pso->SetShader(ShaderType::Pixel, m_grayscaleFragShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);

    DepthStencilState ds;
    ds.depthEnable = false;
    ds.depthWriteEnable = false;
    pso->SetDepthStencilState(ds);

    RasterizerState rs;
    rs.cullEnable = false;
    pso->SetRasterizerState(rs);

    BlendState bs;
    bs.blendEnable = false;
    pso->SetBlendState(bs);

    pso->SetInputLayout({});
    pso->SetRenderTargetFormat(0, TextureFormat::RGBA8_UNorm);

    if (!pso->Create(device)) {
        LOG_ERROR("PostProcess2D", "Failed to create Grayscale PSO: {}", pso->GetErrors());
        return;
    }
    m_grayscalePSO = std::shared_ptr<IPipelineState>(std::move(pso));
    LOG_INFO("PostProcess2D", "Grayscale PSO created");

    // 3. 创建点采样器（最近邻）
    SamplerDesc samplerDesc;
    samplerDesc.filter = TextureFilter::Point;
    samplerDesc.addressU = TextureAddressMode::Clamp;
    samplerDesc.addressV = TextureAddressMode::Clamp;
    samplerDesc.addressW = TextureAddressMode::Clamp;
    m_grayscaleSampler = rm->CreateSampler(samplerDesc);

    // 4. 创建描述符集
    if (m_grayscalePSO) {
        auto& layouts = m_grayscalePSO->GetDescriptorSetLayouts();
        if (!layouts.empty()) {
            m_grayscaleDescSetLayout = layouts[0];
            m_grayscaleDescSet = rf->CreateDescriptorSet(m_grayscaleDescSetLayout.get());
            LOG_INFO("PostProcess2D", "Grayscale descriptor set created");
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// Bloom 资源创建
// ═══════════════════════════════════════════════════════════════
void PostProcessPass2D::EnsureBloomResources(IRenderDevice* device, uint32_t width, uint32_t height) {
    // 如果 PSO 已创建且纹理尺寸匹配，则跳过
    if (m_bloomPSO && m_bloomTempTexture &&
        static_cast<uint32_t>(m_bloomTempTexture->GetWidth()) == width &&
        static_cast<uint32_t>(m_bloomTempTexture->GetHeight()) == height) {
        return;
    }
    if (!device || !m_vertShader) return;

    auto* rf = device->GetResourceFactory();
    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rf || !rm) return;

    // 1. 创建 1/4 大小临时纹理（为未来多通道 Bloom 做准备）
    TextureDesc texDesc;
    texDesc.type = TextureType::Texture2D;
    texDesc.format = TextureFormat::RGBA8_UNorm;
    texDesc.width = width;
    texDesc.height = height;
    texDesc.allowRenderTarget = true;
    texDesc.allowShaderResource = true;
    texDesc.mipLevels = 1;

    auto tempTex = rf->CreateTextureImpl(texDesc);
    if (!tempTex) {
        LOG_ERROR("PostProcess2D", "Failed to create bloom temp texture ({}x{})", width, height);
        return;
    }
    m_bloomTempTexture = std::shared_ptr<ITexture>(std::move(tempTex));
    LOG_INFO("PostProcess2D", "Bloom temp texture created: {}x{}", width, height);

    // 2. 加载 Bloom 着色器
    m_bloomCompositeFragShader = rm->LoadShaderSync("assets/shaders/2d/BloomComposite.frag.spv", "main");
    m_bloomGaussianFragShader = rm->LoadShaderSync("assets/shaders/2d/BloomGaussian.frag.spv", "main");

    if (!m_bloomCompositeFragShader) {
        LOG_ERROR("PostProcess2D", "Failed to load BloomComposite shader");
        return;
    }

    // 3. 创建 Bloom 合成 PSO（单通道 5×5 Gaussian + 叠加）
    auto compPSO = rf->CreatePipelineStateImpl();
    if (!compPSO) { LOG_ERROR("PostProcess2D", "Failed to create Bloom composite PSO impl"); return; }

    compPSO->SetShader(ShaderType::Vertex, m_vertShader);
    compPSO->SetShader(ShaderType::Pixel, m_bloomCompositeFragShader);
    compPSO->SetPrimitiveTopology(PrimitiveTopology::TriangleList);

    DepthStencilState ds;
    ds.depthEnable = false;
    ds.depthWriteEnable = false;
    compPSO->SetDepthStencilState(ds);

    RasterizerState rs;
    rs.cullEnable = false;
    compPSO->SetRasterizerState(rs);

    BlendState bs;
    bs.blendEnable = false;
    compPSO->SetBlendState(bs);

    compPSO->SetInputLayout({});
    compPSO->SetRenderTargetFormat(0, TextureFormat::RGBA8_UNorm);

    if (!compPSO->Create(device)) {
        LOG_ERROR("PostProcess2D", "Failed to create Bloom composite PSO: {}", compPSO->GetErrors());
        return;
    }
    m_bloomPSO = std::shared_ptr<IPipelineState>(std::move(compPSO));
    LOG_INFO("PostProcess2D", "Bloom composite PSO created");

    // 4. 创建 Gaussian 模糊 PSO（为未来多通道使用预留）
    if (m_bloomGaussianFragShader) {
        auto gaussPSO = rf->CreatePipelineStateImpl();
        if (gaussPSO) {
            gaussPSO->SetShader(ShaderType::Vertex, m_vertShader);
            gaussPSO->SetShader(ShaderType::Pixel, m_bloomGaussianFragShader);
            gaussPSO->SetPrimitiveTopology(PrimitiveTopology::TriangleList);

            DepthStencilState gds;
            gds.depthEnable = false;
            gds.depthWriteEnable = false;
            gaussPSO->SetDepthStencilState(gds);

            RasterizerState grs;
            grs.cullEnable = false;
            gaussPSO->SetRasterizerState(grs);

            BlendState gbs;
            gbs.blendEnable = false;
            gaussPSO->SetBlendState(gbs);

            gaussPSO->SetInputLayout({});
            gaussPSO->SetRenderTargetFormat(0, TextureFormat::RGBA8_UNorm);

            if (gaussPSO->Create(device)) {
                m_bloomGaussianPSO = std::shared_ptr<IPipelineState>(std::move(gaussPSO));
                LOG_INFO("PostProcess2D", "Bloom Gaussian PSO created");
            }
        }
    }

    // 5. 创建线性采样器（用于 Bloom）
    SamplerDesc samplerDesc;
    samplerDesc.filter = TextureFilter::Linear;
    samplerDesc.addressU = TextureAddressMode::Clamp;
    samplerDesc.addressV = TextureAddressMode::Clamp;
    samplerDesc.addressW = TextureAddressMode::Clamp;
    m_bloomSampler = rm->CreateSampler(samplerDesc);

    // 6. 创建描述符集（合成 PSO）
    if (m_bloomPSO) {
        auto& layouts = m_bloomPSO->GetDescriptorSetLayouts();
        if (!layouts.empty()) {
            m_bloomDescSetLayout = layouts[0];
            m_bloomDescSet = rf->CreateDescriptorSet(m_bloomDescSetLayout.get());
            LOG_INFO("PostProcess2D", "Bloom descriptor set created");
        }
    }

    // 7. 创建 Gaussian 描述符集（为未来使用预留）
    if (m_bloomGaussianPSO) {
        auto& gaussLayouts = m_bloomGaussianPSO->GetDescriptorSetLayouts();
        if (!gaussLayouts.empty()) {
            m_bloomCompDescSetLayout = gaussLayouts[0];
            m_bloomCompDescSet = rf->CreateDescriptorSet(m_bloomCompDescSetLayout.get());
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// Distortion 资源创建
// ═══════════════════════════════════════════════════════════════
void PostProcessPass2D::EnsureDistortionResources(IRenderDevice* device) {
    if (m_distortionPSO) return;
    if (!device || !m_vertShader) return;

    auto* rf = device->GetResourceFactory();
    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rf || !rm) return;

    // 1. 加载 Distortion 片段着色器
    m_distortionFragShader = rm->LoadShaderSync("assets/shaders/2d/Distortion.frag.spv", "main");
    if (!m_distortionFragShader) {
        LOG_ERROR("PostProcess2D", "Failed to load Distortion shader");
        return;
    }

    // 2. 创建 PSO
    auto pso = rf->CreatePipelineStateImpl();
    if (!pso) { LOG_ERROR("PostProcess2D", "Failed to create Distortion PSO impl"); return; }

    pso->SetShader(ShaderType::Vertex, m_vertShader);
    pso->SetShader(ShaderType::Pixel, m_distortionFragShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);

    DepthStencilState ds;
    ds.depthEnable = false;
    ds.depthWriteEnable = false;
    pso->SetDepthStencilState(ds);

    RasterizerState rs;
    rs.cullEnable = false;
    pso->SetRasterizerState(rs);

    BlendState bs;
    bs.blendEnable = false;
    pso->SetBlendState(bs);

    pso->SetInputLayout({});
    pso->SetRenderTargetFormat(0, TextureFormat::RGBA8_UNorm);

    if (!pso->Create(device)) {
        LOG_ERROR("PostProcess2D", "Failed to create Distortion PSO: {}", pso->GetErrors());
        return;
    }
    m_distortionPSO = std::shared_ptr<IPipelineState>(std::move(pso));
    LOG_INFO("PostProcess2D", "Distortion PSO created");

    // 3. 创建线性采样器
    SamplerDesc samplerDesc;
    samplerDesc.filter = TextureFilter::Linear;
    samplerDesc.addressU = TextureAddressMode::Clamp;
    samplerDesc.addressV = TextureAddressMode::Clamp;
    samplerDesc.addressW = TextureAddressMode::Clamp;
    m_distortionSampler = rm->CreateSampler(samplerDesc);

    // 4. 创建描述符集
    if (m_distortionPSO) {
        auto& layouts = m_distortionPSO->GetDescriptorSetLayouts();
        if (!layouts.empty()) {
            m_distortionDescSetLayout = layouts[0];
            m_distortionDescSet = rf->CreateDescriptorSet(m_distortionDescSetLayout.get());
            LOG_INFO("PostProcess2D", "Distortion descriptor set created");
        }
    }
}

} // namespace Prisma::Graphic
