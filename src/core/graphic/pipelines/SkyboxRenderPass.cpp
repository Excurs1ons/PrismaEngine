#include "SkyboxRenderPass.h"
#include "Logger.h"
#include "ResourceManager.h"
#include "Shader.h"

namespace Engine {
namespace Graphic {
namespace Pipelines {

SkyboxRenderPass::SkyboxRenderPass()
    : m_cubeMapTexture(nullptr)
    , m_renderTarget(nullptr)
    , m_width(0)
    , m_height(0)
{
    InitializeSkyboxMesh();
    // 加载天空盒着色器
    auto resourceManager = ResourceManager::GetInstance();
    auto shaderHandle = resourceManager->Load<Shader>("assets/shaders/Skybox.hlsl");
    if (shaderHandle.IsValid()) {
        m_skyboxShader = std::shared_ptr<Shader>(shaderHandle.Get(), [](Shader*){});
        LOG_DEBUG("SkyboxRenderPass", "天空盒着色器加载成功");
    } else {
        LOG_ERROR("SkyboxRenderPass", "天空盒着色器加载失败");
    }
}

SkyboxRenderPass::~SkyboxRenderPass()
{
}

void SkyboxRenderPass::Execute(RenderCommandContext* context)
{
    LOG_DEBUG("SkyboxRenderPass", "Executing skybox render pass");
    
    // 实现天空盒渲染逻辑
    // 1. 设置天空盒着色器
    // 2. 绑定立方体贴图纹理
    // 3. 设置视图投影矩阵（去掉平移部分）
    // 4. 渲染立方体网格
    
    // 在实际实现中，我们需要访问渲染后端来执行这些操作
    // 这里只是占位符实现
}

void SkyboxRenderPass::SetRenderTarget(void* renderTarget)
{
    m_renderTarget = renderTarget;
    LOG_DEBUG("SkyboxRenderPass", "Setting render target");
}

void SkyboxRenderPass::ClearRenderTarget(float r, float g, float b, float a)
{
    // 天空盒渲染通道不需要清屏操作
    LOG_DEBUG("SkyboxRenderPass", "Ignoring clear render target call");
}

void SkyboxRenderPass::SetViewport(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    LOG_DEBUG("SkyboxRenderPass", "Setting viewport to {0}x{1}", width, height);
}

void SkyboxRenderPass::SetCubeMapTexture(void* cubeMapTexture)
{
    m_cubeMapTexture = cubeMapTexture;
    LOG_DEBUG("SkyboxRenderPass", "Setting cube map texture");
}

void SkyboxRenderPass::SetViewProjectionMatrix(const XMMATRIX& viewProjection)
{
    m_viewProjection = viewProjection;
    LOG_DEBUG("SkyboxRenderPass", "Setting view projection matrix");
    
    // 更新常量缓冲区
    if (m_constantBuffer.empty()) {
        m_constantBuffer.resize(16); // 4x4 矩阵
    }
    
    // 将矩阵数据复制到常量缓冲区
    XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(m_constantBuffer.data()), m_viewProjection);
}

void SkyboxRenderPass::InitializeSkyboxMesh()
{
    // 初始化天空盒立方体网格
    // 创建一个立方体网格用于渲染天空盒
    LOG_DEBUG("SkyboxRenderPass", "Initializing skybox mesh");
    
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
    float vertices[] = {
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
    uint16_t indices[] = {
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
    
    // 在实际实现中，我们将这些数据存储到 m_skyboxMesh 中
    // 这里只是示意性的注释
    LOG_DEBUG("SkyboxRenderPass", "天空盒网格数据已准备: 8个顶点, 36个索引");
}

} // namespace Pipelines
} // namespace Graphic
} // namespace Engine