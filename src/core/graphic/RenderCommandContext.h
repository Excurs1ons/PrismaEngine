#pragma once
#include <../Component.h>
#include <DirectXMath.h>

using namespace DirectX;

// 渲染命令上下文接口
class RenderCommandContext
{
public:
    virtual ~RenderCommandContext() = default;
    
    // 设置常量缓冲区
    virtual void SetConstantBuffer(const char* name, FXMMATRIX matrix) = 0;
    virtual void SetConstantBuffer(const char* name, const float* data, size_t size) = 0;

    // 设置顶点缓冲区（将数据复制到后端的 per-frame upload 区并绑定）
    virtual void SetVertexBuffer(const void* data, uint32_t sizeInBytes, uint32_t strideInBytes) = 0;

    // 设置索引缓冲区（将数据复制到后端的 per-frame upload 区并绑定）
    virtual void SetIndexBuffer(const void* data, uint32_t sizeInBytes, bool use16BitIndices = true) = 0;

    // 设置着色器资源
    virtual void SetShaderResource(const char* name, void* resource) = 0;
    
    // 设置采样器
    virtual void SetSampler(const char* name, void* sampler) = 0;
    
    // 绘制命令
    virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation = 0, uint32_t baseVertexLocation = 0) = 0;
    virtual void Draw(uint32_t vertexCount, uint32_t startVertexLocation = 0) = 0;
    
    // 设置渲染状态
    virtual void SetViewport(float x, float y, float width, float height) = 0;
    virtual void SetScissorRect(int left, int top, int right, int bottom) = 0;
};