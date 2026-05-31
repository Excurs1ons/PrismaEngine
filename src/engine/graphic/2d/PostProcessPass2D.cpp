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

    // 加载通用后处理着色器
    if (!m_vertShader) {
        m_vertShader = rm->LoadShaderSync("assets/shaders/Default.vert.spv", "main");
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
    if (!cmd || !device) return;

    // 仅当 CRT 效果启用时执行
    if (!m_effects[static_cast<int>(EffectType::CRT)]) return;

    EnsureCRTResources(device);
    if (!m_crtPSO || !input) return;

    // 绑定离屏纹理到描述符集
    if (m_crtDescSet && m_crtSampler) {
        m_crtDescSet->BindTexture(0, input, m_crtSampler.get());
        m_crtDescSet->Update();
    }

    // 绑定 PSO
    cmd->SetPipelineState(m_crtPSO.get());

    // 绑定描述符集
    if (m_crtDescSet) {
        cmd->BindDescriptorSet(0, m_crtDescSet.get());
    }

    // 推送 CRT 参数（push constants 在着色器中声明）
    struct {
        float scanlineIntensity;
        float chromaticAberration;
        float brightness;
        float contrast;
        float logicalWidth;
        float logicalHeight;
    } params;

    params.scanlineIntensity = m_crtScanlineIntensity;
    params.chromaticAberration = m_crtChromaticAberration;
    params.brightness = m_crtBrightness;
    params.contrast = m_crtContrast;
    params.logicalWidth = static_cast<float>(input->GetWidth());
    params.logicalHeight = static_cast<float>(input->GetHeight());

    cmd->PushConstants(ShaderType::VertexAndPixel, &params, sizeof(params));

    // 绘制全屏三角形（3 个顶点，gl_VertexIndex，无 VBO）
    cmd->Draw(3, 1, 0);

    LOG_TRACE("PostProcess2D", "CRT processed: {}x{} input (scanline={}, chroma={})",
              input->GetWidth(), input->GetHeight(),
              m_crtScanlineIntensity, m_crtChromaticAberration);
}

} // namespace Prisma::Graphic
