#include "SkyboxRenderPass.h"
#include "Logger.h"
#include "ResourceManager.h"
#include "Shader.h"
#include "DefaultShader.h"
#include <DirectXColors.h>

namespace Engine {
namespace Graphic {
namespace Pipelines {

SkyboxRenderPass::SkyboxRenderPass()
    : m_cubeMapTexture(nullptr)
    , m_renderTarget(nullptr)
    , m_width(0)
    , m_height(0)
{
    LOG_DEBUG("SkyboxRenderPass", "构造函数被调用");
    InitializeSkyboxMesh();

    // 使用硬编码的天空盒着色器
    m_skyboxShader = std::make_shared<Shader>();
    if (!m_skyboxShader || !m_skyboxShader->CompileFromString(Graphic::SKYBOX_VERTEX_SHADER, Graphic::SKYBOX_PIXEL_SHADER)) {
        LOG_ERROR("SkyboxRenderPass", "天空盒着色器编译失败");
        m_skyboxShader.reset();
        return;
    }

    m_skyboxShader->SetName("SkyboxDefault");
    LOG_DEBUG("SkyboxRenderPass", "天空盒着色器编译成功");
}

SkyboxRenderPass::~SkyboxRenderPass()
{
    LOG_DEBUG("SkyboxRenderPass", "析构函数被调用");
}

void SkyboxRenderPass::Execute(RenderCommandContext* context)
{
    LOG_DEBUG("SkyboxRenderPass", "执行 Execute 方法开始");

    if (!context) {
        LOG_WARNING("SkyboxRenderPass", "Render context is null");
        return;
    }

    // 验证必要资源 - 增强的安全检查
    if (!m_skyboxShader) {
        LOG_ERROR("SkyboxRenderPass", "Skybox shader is null, skipping render");
        return;
    }

    if (!m_skyboxShader->IsLoaded()) {
        LOG_ERROR("SkyboxRenderPass", "Skybox shader not loaded, skipping render");
        return;
    }

    if (m_vertices.empty()) {
        LOG_ERROR("SkyboxRenderPass", "Skybox vertices not initialized, skipping render");
        return;
    }

    if (m_indices.empty()) {
        LOG_ERROR("SkyboxRenderPass", "Skybox indices not initialized, skipping render");
        return;
    }

    if (!m_cubeMapTexture) {
        LOG_DEBUG("SkyboxRenderPass", "No cubemap texture set, skipping skybox render");
        return;
    }

    // 检查视口是否有效
    if (m_width == 0 || m_height == 0) {
        LOG_WARNING("SkyboxRenderPass", "Invalid viewport dimensions ({0}x{1}), skipping render", m_width, m_height);
        return;
    }

    LOG_DEBUG("SkyboxRenderPass", "上下文有效，开始渲染逻辑");
    LOG_DEBUG("SkyboxRenderPass", "着色器和网格数据有效");

    try {
        // 1. 设置着色器资源（必须在绘制之前）
        if (m_cubeMapTexture) {
            LOG_DEBUG("SkyboxRenderPass", "设置立方体贴图纹理");
            context->SetShaderResource("CubeMap", m_cubeMapTexture);
        }

        // 2. 设置常量缓冲区（视图投影矩阵）
        if (!m_constantBuffer.empty()) {
            LOG_DEBUG("SkyboxRenderPass", "设置常量缓冲区");
            // 天空盒需要特殊的视图矩阵处理（移除平移）
            XMMATRIX modifiedViewProjection = m_viewProjection;
            modifiedViewProjection.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
            context->SetConstantBuffer("ConstantBuffer", modifiedViewProjection);
        }

        // 3. 设置顶点和索引缓冲区
        LOG_DEBUG("SkyboxRenderPass", "设置顶点缓冲区，顶点数量: {0}", m_vertices.size());
        context->SetVertexBuffer(m_vertices.data(),
                               static_cast<uint32_t>(m_vertices.size() * sizeof(float)),
                               3 * sizeof(float)); // 3个float表示一个顶点位置

        LOG_DEBUG("SkyboxRenderPass", "设置索引缓冲区，索引数量: {0}", m_indices.size());
        context->SetIndexBuffer(m_indices.data(),
                              static_cast<uint32_t>(m_indices.size() * sizeof(uint16_t)),
                              true); // 使用16位索引

        // 4. 绘制立方体
        LOG_DEBUG("SkyboxRenderPass", "调用 DrawIndexed，索引数量: {0}", m_indices.size());
        context->DrawIndexed(static_cast<uint32_t>(m_indices.size()));

        LOG_INFO("SkyboxRenderPass", "天空盒渲染完成");

    } catch (const std::exception& e) {
        LOG_ERROR("SkyboxRenderPass", "Exception during execute: {0}", e.what());
    }
    
    LOG_DEBUG("SkyboxRenderPass", "执行 Execute 方法结束");
}

void SkyboxRenderPass::SetRenderTarget(void* renderTarget)
{
    m_renderTarget = renderTarget;
    LOG_DEBUG("SkyboxRenderPass", "设置渲染目标: 0x{0:x}", reinterpret_cast<uintptr_t>(renderTarget));
}

void SkyboxRenderPass::ClearRenderTarget(float r, float g, float b, float a)
{
    // 天空盒渲染通道不需要清屏操作
    LOG_DEBUG("SkyboxRenderPass", "忽略清屏操作，颜色: ({0}, {1}, {2}, {3})", r, g, b, a);
}

void SkyboxRenderPass::SetViewport(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    LOG_DEBUG("SkyboxRenderPass", "设置视口为 {0}x{1}", width, height);
}

void SkyboxRenderPass::SetDepthBuffer(void* depthBuffer)
{
    // 天空盒不需要特殊的深度缓冲处理，但需要保存以避免编译警告
    (void)depthBuffer;
    LOG_DEBUG("SkyboxRenderPass", "设置深度缓冲区: 0x{0:x}", reinterpret_cast<uintptr_t>(depthBuffer));
}

void SkyboxRenderPass::SetCubeMapTexture(void* cubeMapTexture)
{
    m_cubeMapTexture = cubeMapTexture;
    LOG_DEBUG("SkyboxRenderPass", "设置立方体贴图纹理: 0x{0:x}", reinterpret_cast<uintptr_t>(cubeMapTexture));
}

void SkyboxRenderPass::SetViewProjectionMatrix(const XMMATRIX& viewProjection)
{
    m_viewProjection = viewProjection;
    LOG_DEBUG("SkyboxRenderPass", "设置视图投影矩阵");

    // 更新常量缓冲区
    if (m_constantBuffer.empty()) {
        m_constantBuffer.resize(16); // 4x4 矩阵
    }

    // 将矩阵数据复制到常量缓冲区
    XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(m_constantBuffer.data()), m_viewProjection);
}

void SkyboxRenderPass::SetViewMatrix(const XMMATRIX& view)
{
    m_view = view;
    // 重新计算视图投影矩阵
    m_viewProjection = m_view * m_projection;

    // 更新常量缓冲区
    if (m_constantBuffer.empty()) {
        m_constantBuffer.resize(16); // 4x4 矩阵
    }

    XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(m_constantBuffer.data()), m_viewProjection);
}

void SkyboxRenderPass::SetProjectionMatrix(const XMMATRIX& projection)
{
    m_projection = projection;
    // 重新计算视图投影矩阵
    m_viewProjection = m_view * m_projection;

    // 更新常量缓冲区
    if (m_constantBuffer.empty()) {
        m_constantBuffer.resize(16); // 4x4 矩阵
    }

    XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(m_constantBuffer.data()), m_viewProjection);
}

void SkyboxRenderPass::InitializeSkyboxMesh()
{
    // 初始化天空盒立方体网格
    // 创建一个立方体网格用于渲染天空盒
    LOG_DEBUG("SkyboxRenderPass", "初始化天空盒网格");
    
    // 在实际实现中，我们会在这里创建立方体顶点和索引数据
    // 并将其存储在 m_skyboxMesh 中
    
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
    
    // 顶点数据
    m_vertices = {
        // 前面
        -1.0f, -1.0f,  1.0f,  // 0
         1.0f, -1.0f,  1.0f,  // 1
         1.0f,  1.0f,  1.0f,  // 2
        -1.0f,  1.0f,  1.0f,  // 3
        // 后面
        -1.0f, -1.0f, -1.0f,  // 4
         1.0f, -1.0f, -1.0f,  // 5
         1.0f,  1.0f, -1.0f,  // 6
        -1.0f,  1.0f, -1.0f,  // 7
    };
    
    // 索引数据 (三角形列表)
    m_indices = {
        // 前面
        0, 1, 2,
        2, 3, 0,
        // 后面
        6, 5, 4,
        4, 7, 6,
        // 左面
        4, 0, 3,
        3, 7, 4,
        // 右面
        1, 5, 6,
        6, 2, 1,
        // 上面
        3, 2, 6,
        6, 7, 3,
        // 下面
        4, 5, 1,
        1, 0, 4
    };
    
    LOG_INFO("SkyboxRenderPass", "天空盒网格数据已准备: {0}个顶点, {1}个索引",
              m_vertices.size() / 3, m_indices.size());
}

bool SkyboxRenderPass::IsInitialized() const {
    return m_skyboxShader != nullptr &&
           m_skyboxShader->IsLoaded() &&
           !m_vertices.empty() &&
           !m_indices.empty() &&
           m_width > 0 &&
           m_height > 0;
}

} // namespace Pipelines
} // namespace Graphic
} // namespace Engine