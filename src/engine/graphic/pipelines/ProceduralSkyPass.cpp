#include "ProceduralSkyPass.h"
#include "app/Engine.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "logger/Logger.h"

namespace Prisma::Graphic {

ProceduralSkyPass::ProceduralSkyPass()
    : ForwardRenderPass("ProceduralSkyPass") {
    // 天空在不透明物体之后渲染
    m_priority = 200;
    InitializeSkyboxMesh();
}

void ProceduralSkyPass::Update(Timestep ts) {
    UpdateTime(ts);
}

bool ProceduralSkyPass::Initialize(IRenderDevice* device) {
    if (!device) return false;
    m_device = device;

    auto rm = Engine::Get().GetRenderResourceManager();
    if (!rm) {
        LOG_ERROR("ProceduralSkyPass", "RenderResourceManager 不可用");
        return false;
    }

    m_vertexShader = rm->LoadShaderSync("assets/shaders/procedural_sky.vert.spv");
    m_fragmentShader = rm->LoadShaderSync("assets/shaders/procedural_sky.frag.spv");
    if (!m_vertexShader || !m_fragmentShader) {
        LOG_ERROR("ProceduralSkyPass", "无法加载 procedural_sky shader");
        return false;
    }

    auto pso = m_device->GetResourceFactory()->CreatePipelineStateImpl();
    if (!pso) return false;
    pso->SetShader(ShaderType::Vertex, m_vertexShader);
    pso->SetShader(ShaderType::Pixel, m_fragmentShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    RasterizerState rs; rs.cullMode = CullMode::None;
    pso->SetRasterizerState(rs);
    DepthStencilState ds{}; ds.depthEnable = false; ds.depthWriteEnable = false;
    pso->SetDepthStencilState(ds);
    if (!pso->Create(m_device)) {
        LOG_ERROR("ProceduralSkyPass", "PSO 创建失败");
        return false;
    }
    m_pso = std::shared_ptr<IPipelineState>(std::move(pso));
    m_psoReady = true;
    LOG_INFO("ProceduralSkyPass", "ProceduralSkyPass 初始化完成");
    return true;
}

void ProceduralSkyPass::Shutdown() {
    m_pso.reset();
    m_vertexShader.reset();
    m_fragmentShader.reset();
    m_psoReady = false;
    m_device = nullptr;
}

void ProceduralSkyPass::Execute(const PassExecutionContext& context) {
    if (!context.deviceContext || !m_meshInitialized || !m_psoReady || !m_pso) {
        return;
    }

    // 视口
    context.deviceContext->SetViewport(0.0f, 0.0f,
        static_cast<float>(context.sceneData->viewport.width),
        static_cast<float>(context.sceneData->viewport.height));

    // 天空盒需要特殊的视图投影矩阵（移除平移部分）
    PrismaMath::mat4 modifiedViewProjection = m_viewProjection;
    modifiedViewProjection[3] = PrismaMath::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    // 绑定自有 PSO
    context.deviceContext->SetPipelineState(m_pso.get());

    // UBO: viewProj + sun params
    SkyUBO ubo{};
    ubo.viewProj = modifiedViewProjection;
    ubo.sunDirIntensity = PrismaMath::vec4(m_sunDirection, m_sunIntensity);
    ubo.sunColor = PrismaMath::vec4(m_sunColor, 0.0f);
    context.deviceContext->SetConstantData(0, &ubo, sizeof(SkyUBO));

    // mesh
    context.deviceContext->SetVertexData(
        m_vertices.data(),
        static_cast<uint32_t>(m_vertices.size() * sizeof(Vertex)),
        static_cast<uint32_t>(sizeof(Vertex)));
    context.deviceContext->SetIndexData(
        m_indices.data(),
        static_cast<uint32_t>(m_indices.size() * sizeof(uint32_t)),
        false);
    context.deviceContext->DrawIndexed(static_cast<uint32_t>(m_indices.size()));
}

void ProceduralSkyPass::InitializeSkyboxMesh() {
    // [复制自 SkyboxRenderPass.cpp:84-178]
    m_vertices.resize(24);

    // 前面 (Z+)
    m_vertices[0].position = PrismaMath::vec4(-1.0f, -1.0f,  1.0f, 1.0f);
    m_vertices[1].position = PrismaMath::vec4( 1.0f, -1.0f,  1.0f, 1.0f);
    m_vertices[2].position = PrismaMath::vec4( 1.0f,  1.0f,  1.0f, 1.0f);
    m_vertices[3].position = PrismaMath::vec4(-1.0f,  1.0f,  1.0f, 1.0f);

    // 后面 (Z-)
    m_vertices[4].position = PrismaMath::vec4(-1.0f, -1.0f, -1.0f, 1.0f);
    m_vertices[5].position = PrismaMath::vec4( 1.0f, -1.0f, -1.0f, 1.0f);
    m_vertices[6].position = PrismaMath::vec4( 1.0f,  1.0f, -1.0f, 1.0f);
    m_vertices[7].position = PrismaMath::vec4(-1.0f,  1.0f, -1.0f, 1.0f);

    // 左面 (X-)
    m_vertices[8].position  = PrismaMath::vec4(-1.0f, -1.0f, -1.0f, 1.0f);
    m_vertices[9].position  = PrismaMath::vec4(-1.0f, -1.0f,  1.0f, 1.0f);
    m_vertices[10].position = PrismaMath::vec4(-1.0f,  1.0f,  1.0f, 1.0f);
    m_vertices[11].position = PrismaMath::vec4(-1.0f,  1.0f, -1.0f, 1.0f);

    // 右面 (X+)
    m_vertices[12].position = PrismaMath::vec4( 1.0f, -1.0f,  1.0f, 1.0f);
    m_vertices[13].position = PrismaMath::vec4( 1.0f, -1.0f, -1.0f, 1.0f);
    m_vertices[14].position = PrismaMath::vec4( 1.0f,  1.0f, -1.0f, 1.0f);
    m_vertices[15].position = PrismaMath::vec4( 1.0f,  1.0f,  1.0f, 1.0f);

    // 上面 (Y+)
    m_vertices[16].position = PrismaMath::vec4(-1.0f,  1.0f,  1.0f, 1.0f);
    m_vertices[17].position = PrismaMath::vec4( 1.0f,  1.0f,  1.0f, 1.0f);
    m_vertices[18].position = PrismaMath::vec4( 1.0f,  1.0f, -1.0f, 1.0f);
    m_vertices[19].position = PrismaMath::vec4(-1.0f,  1.0f, -1.0f, 1.0f);

    // 下面 (Y-)
    m_vertices[20].position = PrismaMath::vec4(-1.0f, -1.0f, -1.0f, 1.0f);
    m_vertices[21].position = PrismaMath::vec4( 1.0f, -1.0f, -1.0f, 1.0f);
    m_vertices[22].position = PrismaMath::vec4( 1.0f, -1.0f,  1.0f, 1.0f);
    m_vertices[23].position = PrismaMath::vec4(-1.0f, -1.0f,  1.0f, 1.0f);

    // 法线 + 颜色 + tangent
    for (auto& v : m_vertices) {
        v.normal = PrismaMath::vec4(v.position.x, v.position.y, v.position.z, 0.0f);
        v.color = PrismaMath::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        v.tangent = PrismaMath::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    // 纹理坐标
    m_vertices[0].texCoord  = PrismaMath::vec4( 1.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[1].texCoord  = PrismaMath::vec4( 1.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[2].texCoord  = PrismaMath::vec4( 0.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[3].texCoord  = PrismaMath::vec4( 0.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[4].texCoord  = PrismaMath::vec4( 1.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[5].texCoord  = PrismaMath::vec4( 0.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[6].texCoord  = PrismaMath::vec4( 0.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[7].texCoord  = PrismaMath::vec4( 1.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[8].texCoord  = PrismaMath::vec4( 0.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[9].texCoord  = PrismaMath::vec4( 1.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[10].texCoord = PrismaMath::vec4( 1.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[11].texCoord = PrismaMath::vec4( 0.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[12].texCoord = PrismaMath::vec4( 1.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[13].texCoord = PrismaMath::vec4( 0.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[14].texCoord = PrismaMath::vec4( 0.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[15].texCoord = PrismaMath::vec4( 1.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[16].texCoord = PrismaMath::vec4( 0.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[17].texCoord = PrismaMath::vec4( 1.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[18].texCoord = PrismaMath::vec4( 1.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[19].texCoord = PrismaMath::vec4( 0.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[20].texCoord = PrismaMath::vec4( 1.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[21].texCoord = PrismaMath::vec4( 0.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[22].texCoord = PrismaMath::vec4( 0.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[23].texCoord = PrismaMath::vec4( 1.0f,  1.0f,  0.0f, 0.0f);

    // 索引数据 (三角形列表)
    m_indices = {
        0, 1, 2,  2, 3, 0,
        6, 5, 4,  4, 7, 6,
        4, 0, 3,  3, 7, 4,
        1, 5, 6,  6, 2, 1,
        3, 2, 6,  6, 7, 3,
        4, 5, 1,  1, 0, 4
    };

    m_meshInitialized = true;
}

} // namespace Prisma::Graphic
