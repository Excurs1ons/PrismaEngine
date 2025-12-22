#pragma once
#include "math/MathTypes.h"
#include <string>
// 渲染命令上下文接口
class RenderCommandContext
{
public:
    virtual ~RenderCommandContext() = default;

    // 设置常量缓冲区
    virtual void SetConstantBuffer(const std::string& name, Prisma::Matrix4x4 matrix) = 0;
    virtual void SetConstantBuffer(const std::string& name, const float* data, size_t size) = 0;

    // 设置顶点缓冲区（将数据复制到后端的 per-frame upload 区并绑定）
    virtual void SetVertexBuffer(const void* data, uint32_t sizeInBytes, uint32_t strideInBytes) = 0; // 设置顶点缓冲区（将数据复制到后端的 per-frame upload 区并绑定）

    // 设置索引缓冲区（将数据复制到后端的 per-frame upload 区并绑定）
    virtual void SetIndexBuffer(const void* data, uint32_t sizeInBytes, bool use16BitIndices = true) = 0; // 设置索引缓冲区（将数据复制到后端的 per-frame upload 区并绑定）

    // 设置着色器资源
    virtual void SetShaderResource(const std::string& name, void* resource) = 0; // 设置着色器资源

    // 设置采样器
    virtual void SetSampler(const std::string& name, void* sampler) = 0; // 设置采样器

    // 绘制命令
    virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation = 0, uint32_t baseVertexLocation = 0) = 0; // 绘制命令
    virtual void Draw(uint32_t vertexCount, uint32_t startVertexLocation = 0) = 0; // 绘制命令

    // 设置渲染状态
    virtual void SetViewport(float x, float y, float width, float height) = 0; // 设置渲染状态
    virtual void SetScissorRect(int left, int top, int right, int bottom) = 0; // 设置渲染状态

    // 设置管线状态（PSO）- 仅适用于需要显式PSO管理的后端（如DirectX 12）
    virtual void SetPipelineState(void* pso) {}
};