#include "SkyboxRenderPass.h"
#include <iostream>

namespace PrismaEngine::Graphic {

SkyboxPass::SkyboxPass()
    : ForwardRenderPass("SkyboxPass")
    , m_cubeMapTexture(nullptr)
    , m_initialized(false)
    , m_meshInitialized(false) {
    // 天空盒在不透明物体之后渲染，但在透明物体之前
    m_priority = 200;
    InitializeSkyboxMesh();
}

void SkyboxPass::Update(float deltaTime) {
    UpdateTime(deltaTime);
}

void SkyboxPass::Execute(const PassExecutionContext& context) {
    if (!context.deviceContext) {
        return;
    }

    // 验证必要资源
    if (!m_meshInitialized) {
        return;
    }

    // 设置视口
    context.deviceContext->SetViewport(0.0f, 0.0f,
        static_cast<float>(context.sceneData->viewport.width),
        static_cast<float>(context.sceneData->viewport.height));

    // 天空盒需要特殊的视图投影矩阵（移除平移部分）
    PrismaMath::mat4 modifiedViewProjection = m_viewProjection;
    modifiedViewProjection[3] = PrismaMath::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    // 设置常量数据（视图投影矩阵）
    context.deviceContext->SetConstantData(0, &modifiedViewProjection, sizeof(PrismaMath::mat4));

    // 设置顶点数据
    context.deviceContext->SetVertexData(
        m_vertices.data(),
        static_cast<uint32_t>(m_vertices.size() * sizeof(Vertex)),
        static_cast<uint32_t>(sizeof(Vertex))
    );

    // 设置索引数据
    context.deviceContext->SetIndexData(
        m_indices.data(),
        static_cast<uint32_t>(m_indices.size() * sizeof(uint32_t)),
        false  // 32 位索引
    );

    // 设置立方体贴图纹理
    if (m_cubeMapTexture) {
        context.deviceContext->SetTexture(m_cubeMapTexture, 0);
    }

    // 绘制
    context.deviceContext->DrawIndexed(static_cast<uint32_t>(m_indices.size()));
}

void SkyboxPass::InitializeSkyboxMesh() {
    // 定义立方体的8个顶点
    // 前面 (Z+)
    // 1----2
    // |\   |
    // | \  |
    // |  \ |
    // |   \|
    // 0----3

    // 后面 (Z-)
    // 5----6
    // |\   |
    // | \  |
    // |  \ |
    // |   \|
    // 4----7

    // 创建立方体顶点
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

    // 设置法线（指向立方体中心外）
    for (auto& v : m_vertices) {
        v.normal = PrismaMath::vec4(v.position.x, v.position.y, v.position.z, 0.0f);
    }

    // 设置纹理坐标（立方体贴图采样）
    m_vertices[0].texCoord  = PrismaMath::vec4( 1.0f,  0.0f,  0.0f, 0.0f);  // 前面
    m_vertices[1].texCoord  = PrismaMath::vec4( 1.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[2].texCoord  = PrismaMath::vec4( 0.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[3].texCoord  = PrismaMath::vec4( 0.0f,  0.0f,  0.0f, 0.0f);

    m_vertices[4].texCoord  = PrismaMath::vec4( 1.0f,  0.0f,  0.0f, 0.0f);  // 后面
    m_vertices[5].texCoord  = PrismaMath::vec4( 0.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[6].texCoord  = PrismaMath::vec4( 0.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[7].texCoord  = PrismaMath::vec4( 1.0f,  1.0f,  0.0f, 0.0f);

    m_vertices[8].texCoord  = PrismaMath::vec4( 0.0f,  0.0f,  0.0f, 0.0f);  // 左面
    m_vertices[9].texCoord  = PrismaMath::vec4( 1.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[10].texCoord = PrismaMath::vec4( 1.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[11].texCoord = PrismaMath::vec4( 0.0f,  1.0f,  0.0f, 0.0f);

    m_vertices[12].texCoord = PrismaMath::vec4( 1.0f,  0.0f,  0.0f, 0.0f);  // 右面
    m_vertices[13].texCoord = PrismaMath::vec4( 0.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[14].texCoord = PrismaMath::vec4( 0.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[15].texCoord = PrismaMath::vec4( 1.0f,  1.0f,  0.0f, 0.0f);

    m_vertices[16].texCoord = PrismaMath::vec4( 0.0f,  1.0f,  0.0f, 0.0f);  // 上面
    m_vertices[17].texCoord = PrismaMath::vec4( 1.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[18].texCoord = PrismaMath::vec4( 1.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[19].texCoord = PrismaMath::vec4( 0.0f,  0.0f,  0.0f, 0.0f);

    m_vertices[20].texCoord = PrismaMath::vec4( 1.0f,  0.0f,  0.0f, 0.0f);  // 下面
    m_vertices[21].texCoord = PrismaMath::vec4( 0.0f,  0.0f,  0.0f, 0.0f);
    m_vertices[22].texCoord = PrismaMath::vec4( 0.0f,  1.0f,  0.0f, 0.0f);
    m_vertices[23].texCoord = PrismaMath::vec4( 1.0f,  1.0f,  0.0f, 0.0f);

    // 设置颜色（白色）
    for (auto& v : m_vertices) {
        v.color = PrismaMath::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        v.tangent = PrismaMath::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    // 索引数据 (三角形列表)
    m_indices = {
        // 前面
        0, 1, 2,  2, 3, 0,
        // 后面
        6, 5, 4,  4, 7, 6,
        // 左面
        4, 0, 3,  3, 7, 4,
        // 右面
        1, 5, 6,  6, 2, 1,
        // 上面
        3, 2, 6,  6, 7, 3,
        // 下面
        4, 5, 1,  1, 0, 4
    };

    m_meshInitialized = true;
    m_initialized = true;
}

} // namespace PrismaEngine::Graphic
