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
    // 默认不通过此接口执行，而是由 Pipeline 显式调用 Process
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

void PostProcessPass2D::Process(ICommandBuffer* cmd, IRenderDevice* device, ITexture* input, IRenderTarget* output) {
    if (!cmd || !device || !input) return;

    EnsureResources(input->GetWidth(), input->GetHeight(), device);
    if (!m_pso) return;

    // 设置渲染目标
    if (output) {
        // TODO: 绑定到指定的 RT
    }

    cmd->SetPipelineState(m_pso.get());
    // 绑定输入纹理到槽位 0
    // cmd->BindTexture(0, input);
    
    // 绘制全屏三角形
    // cmd->Draw(3);
}

} // namespace Prisma::Graphic
