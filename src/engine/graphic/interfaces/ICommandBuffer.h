#pragma once

#include "RenderTypes.h"
#include "IPipelineState.h"
#include "IDescriptorSet.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class IBuffer;
class ITexture;
class ISampler;

struct RenderPassDesc {
    ITexture* renderTarget = nullptr;
    ITexture* depthStencil = nullptr;
    Color clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    float clearDepthValue = 1.0f;
    uint8_t clearStencilValue = 0;
    Rect renderArea = {0, 0, 0, 0};
    bool clearRenderTarget = true;
    bool clearDepth = true;
    bool clearStencil = true;
};

/**
 * @brief 命令缓冲区抽象接口 (现代化版本)
 */
class ICommandBuffer {
public:
    virtual ~ICommandBuffer() = default;

    // === 生命周期 ===
    virtual void Begin() = 0;
    virtual void End() = 0;
    virtual bool Reset() = 0;

    // === 渲染通道 ===
    virtual void BeginRenderPass(const RenderPassDesc& desc) = 0;
    virtual void EndRenderPass() = 0;

    // === 状态与管线 ===
    virtual void SetPipelineState(IPipelineState* pipelineState) = 0;
    virtual void SetViewport(const Viewport& viewport) = 0;
    virtual void SetScissorRect(const Rect& rect) = 0;

    // === 资源绑定 (核心重构) ===
    virtual void SetVertexBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset = 0) = 0;
    virtual void SetIndexBuffer(IBuffer* buffer, bool is32Bit = true, uint32_t offset = 0) = 0;

    /**
     * @brief 绑定描述符集 (Vulkan Set / DX12 Table)
     * @param set 索引 (0: Global, 1: Material, 2: Per-Object)
     */
    virtual void BindDescriptorSet(uint32_t set, IDescriptorSet* descriptorSet) = 0;

    /**
     * @brief 推流常量 (用于频繁更新的变换矩阵)
     */
    virtual void PushConstants(ShaderType stage, const void* data, uint32_t size) = 0;

    // === 绘制命令 ===
    virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0) = 0;
    virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0) = 0;

    // === 间接绘制与计算 ===
    virtual void Dispatch(uint32_t x, uint32_t y, uint32_t z) = 0;
    virtual void DrawIndexedIndirect(IBuffer* indirectBuffer, uint32_t offset = 0) = 0;

    // === 资源同步与屏障 ===
    virtual void PipelineBarrier() = 0; // 抽象的屏障接口

    // === 调试 ===
    virtual void BeginDebugGroup(const std::string& name) = 0;
    virtual void EndDebugGroup() = 0;
};

} // namespace Prisma::Graphic
