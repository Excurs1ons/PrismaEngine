#pragma once

#include "RenderPass.h"
#include <vector>
#include <memory>

// 前向声明
class Mesh;

namespace Engine {

// 几何渲染通道类
class GeometryRenderPass : public RenderPass
{
public:
    GeometryRenderPass();
    ~GeometryRenderPass();
    
    // 渲染通道执行函数
    void Execute(RenderCommandContext* context) override;
    
    // 设置渲染目标
    void SetRenderTarget(void* renderTarget) override;
    
    // 清屏操作
    void ClearRenderTarget(float r, float g, float b, float a) override;
    
    // 设置视口
    void SetViewport(uint32_t width, uint32_t height) override;
    
    // 添加网格到渲染队列
    void AddMeshToRenderQueue(std::shared_ptr<Mesh> mesh, const float* transform);
    
private:
    // 渲染目标
    void* m_renderTarget;
    
    // 清屏颜色
    float m_clearColor[4];
    
    // 视口尺寸
    uint32_t m_width;
    uint32_t m_height;
    
    // 待渲染的网格队列
    struct RenderItem {
        std::shared_ptr<Mesh> mesh;
        float transform[16]; // 4x4变换矩阵
    };
    
    std::vector<RenderItem> m_renderQueue;
};

} // namespace Engine