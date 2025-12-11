#pragma once

#include "graphic/RenderPass.h"
#include "graphic/Material.h"
#include "graphic/Mesh.h"
#include <memory>
#include <vector>

namespace Engine {

class Shader; // 前向声明

namespace Graphic {
namespace Pipelines {

// 天空盒渲染通道
class SkyboxRenderPass : public RenderPass
{
public:
    SkyboxRenderPass();
    ~SkyboxRenderPass();
    
    // 渲染通道执行函数
    void Execute(RenderCommandContext* context) override;
    
    // 设置渲染目标
    void SetRenderTarget(void* renderTarget) override;
    
    // 清屏操作
    void ClearRenderTarget(float r, float g, float b, float a) override;
    
    // 设置视口
    void SetViewport(uint32_t width, uint32_t height) override;
    
    // 设置天空盒立方体贴图
    void SetCubeMapTexture(void* cubeMapTexture);
    
    // 设置视图投影矩阵
    void SetViewProjectionMatrix(const XMMATRIX& viewProjection);

private:
    // 天空盒立方体贴图纹理
    void* m_cubeMapTexture;
    
    // 视图投影矩阵
    XMMATRIX m_viewProjection;
    
    // 渲染目标
    void* m_renderTarget;
    
    // 视口尺寸
    uint32_t m_width;
    uint32_t m_height;
    
    // 天空盒网格和材质
    std::shared_ptr<Mesh> m_skyboxMesh;
    std::shared_ptr<Material> m_skyboxMaterial;
    std::shared_ptr<Shader> m_skyboxShader;
    
    // 网格数据
    std::vector<float> m_vertices;
    std::vector<uint16_t> m_indices;
    
    // 常量缓冲区数据
    std::vector<float> m_constantBuffer;
    
    // 初始化天空盒网格
    void InitializeSkyboxMesh();
};

} // namespace Pipelines
} // namespace Graphic
} // namespace Engine