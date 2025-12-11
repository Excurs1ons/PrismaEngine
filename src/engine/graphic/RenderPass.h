#pragma once

#include <memory>
#include "RenderCommandContext.h"

// 前向声明
class Mesh;

class RenderPass
{
public:
    RenderPass();
    virtual ~RenderPass();
    
    // 渲染通道执行函数
    virtual void Execute(RenderCommandContext* context) = 0;
    
    // 设置渲染目标
    virtual void SetRenderTarget(void* renderTarget) = 0;
    
    // 清屏操作
    virtual void ClearRenderTarget(float r, float g, float b, float a) = 0;
    
    // 设置视口
    virtual void SetViewport(uint32_t width, uint32_t height) = 0;
};

// 2D渲染通道类
class RenderPass2D : public RenderPass
{
public:
    RenderPass2D();
    ~RenderPass2D();
    
    // 2D渲染通道执行函数
    void Execute(RenderCommandContext* context) override;
    
    // 添加2D网格到渲染队列
    void AddMeshToRenderQueue(std::shared_ptr<Mesh> mesh, FXMMATRIX transform);
    
    // 设置摄像机矩阵
    void SetCameraMatrix(FXMMATRIX viewProjection);
    
    // 设置视口
    void SetViewport(uint32_t width, uint32_t height) override;
    
private:
    // 2D渲染相关的私有数据
    XMMATRIX m_cameraMatrix;
    uint32_t m_width;
    uint32_t m_height;
};