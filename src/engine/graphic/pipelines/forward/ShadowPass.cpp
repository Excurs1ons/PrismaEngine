#include "graphic/pipelines/forward/ShadowPass.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/ISampler.h"
#include <algorithm>
#include <vector>

namespace Prisma::Graphic {

// ============================================================================
// ShadowPass
// ============================================================================

ShadowPass::ShadowPass()
    : ForwardRenderPass("ShadowPass")
    , m_shadowMapManager(nullptr)
    , m_cascadeCount(4)
    , m_pcfKernelSize(4)
    , m_device(nullptr)
    , m_factory(nullptr)
    , m_ready(false) {

    // 阴影 Pass 应在 OpaquePass 之前执行
    SetPriority(10);
}

ShadowPass::~ShadowPass() {
    Cleanup();
}

void ShadowPass::Execute(const PassExecutionContext& context) {
    if (!m_ready || !m_device || !context.deviceContext) return;

    // 注意: IDeviceContext 不提供 ICommandBuffer 转换，
    // 真正的阴影渲染通过 Execute(ICommandBuffer*, const std::vector<ShadowLight>&) 执行。
    // ForwardPipeline 直接调用 ICommandBuffer* 版本，不走此路径。
    // 此处仅执行基本的设备上下文设置操作。
    if (context.sceneData) {
        context.deviceContext->SetViewport(
            0.0f, 0.0f,
            static_cast<float>(context.sceneData->viewport.width),
            static_cast<float>(context.sceneData->viewport.height)
        );
    }
}

void ShadowPass::Update(Prisma::Timestep ts) {
    // 更新基类时间状态
    UpdateTime(ts);
}

bool ShadowPass::Setup(IRenderDevice* device, IResourceFactory* factory) {
    if (!device || !factory) {
        return false;
    }

    m_device = device;
    m_factory = factory;

    // 创建阴影深度 PSO
    if (!EnsurePipeline()) {
        return false;
    }

    m_ready = true;
    return true;
}

void ShadowPass::Cleanup() {
    m_vertexShader.reset();
    m_pixelShader.reset();
    m_pipelineState.reset();
    m_ready = false;
    m_device = nullptr;
    m_factory = nullptr;

    // 注意: 不释放 ShadowMapManager 的资源 (由外部管理)
    m_shadowMapManager = nullptr;
}

void ShadowPass::Execute(ICommandBuffer* cmd, const std::vector<ShadowLight>& lights) {
    if (!m_ready || !cmd || !m_shadowMapManager || !m_shadowMapManager->IsValid()) {
        return;
    }

    if (!m_pipelineState) return;

    // 设置阴影深度渲染管线状态
    cmd->SetPipelineState(m_pipelineState.get());

    uint32_t cascadeCount = m_shadowMapManager->GetCascadeCount();
    if (cascadeCount == 0) return;

    // 遍历每个光源
    for (uint32_t lightIdx = 0; lightIdx < lights.size(); ++lightIdx) {
        const auto& light = lights[lightIdx];
        if (!light.castShadows) continue;

        // 获取光源的阴影贴图纹理数组
        ITexture* shadowMapArray = m_shadowMapManager->GetShadowMapArray(lightIdx);
        if (!shadowMapArray) continue;

        // 获取光源阴影贴图的总数
        uint32_t actualCascades = std::min(cascadeCount, ShadowMapManager::kMaxCascades);

        // 为每个级联渲染阴影贴图
        for (uint32_t cascadeIdx = 0; cascadeIdx < actualCascades; ++cascadeIdx) {
            // 获取该级联的视投影矩阵
            const PrismaMath::mat4& lightVP = m_shadowMapManager->GetShadowMatrix(lightIdx, cascadeIdx);

            // 设置视口 (全分辨率)
            uint32_t shadowSize = m_shadowMapManager->GetShadowMapSize();
            Viewport viewport;
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(shadowSize);
            viewport.height = static_cast<float>(shadowSize);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            cmd->SetViewport(viewport);

            Rect scissor;
            scissor.x = 0;
            scissor.y = 0;
            scissor.width = static_cast<int>(shadowSize);
            scissor.height = static_cast<int>(shadowSize);
            cmd->SetScissorRect(scissor);

            // 开始渲染到该级联切片
            // 注意: 使用 Texture2DArray 时，通过 arraySlice 或 geometry shader 选择切片
            // 此处使用单独的 render pass 通过 arraySlice 视图渲染到目标级联

            // 准备渲染通道描述
            RenderPassDesc rpDesc;
            rpDesc.renderTarget = nullptr;           // 无颜色输出
            rpDesc.depthStencil = shadowMapArray;    // 深度模板目标
            rpDesc.clearDepthValue = 1.0f;
            rpDesc.clearStencilValue = 0;
            rpDesc.renderArea.x = 0;
            rpDesc.renderArea.y = 0;
            rpDesc.renderArea.width = static_cast<int>(shadowSize);
            rpDesc.renderArea.height = static_cast<int>(shadowSize);
            rpDesc.clearRenderTarget = false;
            rpDesc.clearDepth = true;
            rpDesc.clearStencil = false;

            // 使用场景渲染命令
            // RenderCascade(cmd, light, lightVP, cascadeIdx, geometry);

            // 设置 push constants
            ShadowPushConstants pushConsts;
            pushConsts.lightVP = lightVP;
            pushConsts.shadowBias = light.shadowBias;
            pushConsts.shadowNormalBias = light.shadowNormalBias;
            pushConsts.cascadeIndex = cascadeIdx;
            pushConsts.cascadeCount = static_cast<float>(actualCascades);

            cmd->PushConstants(ShaderType::Vertex, &pushConsts, sizeof(ShadowPushConstants));
        }
    }
}

bool ShadowPass::EnsurePipeline() {
    if (!m_device || !m_factory) return false;

    // 阴影深度管线使用独立的 PSO
    // 此处由具体实现根据后端创建
    // 典型的阴影深度 PSO 包含:
    //   - 顶点着色器: 转换到光照空间 (使用 lightVP)
    //   - 像素着色器: 空 (仅深度输出) 或 简单深度输出
    //   - 深度测试: LESS, 深度写入: ON
    //   - 裁剪: CULL_BACK (或 CULL_FRONT 取决于实现)
    //   - 颜色混合: 关闭

    // 此管线的实际创建由具体的后端渲染器完成
    // 这里预留 PSO 创建位置，着色器加载由外部或工厂完成

    // 创建阴影深度着色器 (占位)
    ShaderDesc vertexDesc;
    vertexDesc.type = ShaderType::Vertex;
    vertexDesc.name = "ShadowPass_VS";
    // 实际着色器从资源加载
    // m_vertexShader = m_factory->CreateShaderImpl(vertexDesc, ...);

    ShaderDesc pixelDesc;
    pixelDesc.type = ShaderType::Pixel;
    pixelDesc.name = "ShadowPass_PS";
    // m_pixelShader = m_factory->CreateShaderImpl(pixelDesc, ...);

    return true;
}

void ShadowPass::RenderCascade(ICommandBuffer* cmd, const ShadowLight& light,
                                const PrismaMath::mat4& lightVP, uint32_t cascadeIndex,
                                const std::vector<RenderCommand>& geometry) {
    // 实际渲染几何体到阴影深度缓冲
    // 设置视口到级联大小
    // 绑定深度模板目标
    // 遍历几何体绘制调用

    // 此实现将在 T8 (渲染基础设施集成) 中完成
    // 当前为框架预留接口
}

} // namespace Prisma::Graphic
