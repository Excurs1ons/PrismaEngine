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
class IComputePipeline;

// === Pipeline Barrier 类型定义 ===
enum class ResourceState {
    Undefined,
    Common,
    ShaderRead,
    UnorderedAccess,
    CopySrc,
    CopyDst,
    RenderTarget,
    DepthStencil,
    Present,
};

struct ImageBarrier {
    ITexture* texture = nullptr;
    ResourceState oldState = ResourceState::Undefined;
    ResourceState newState = ResourceState::Common;
    uint32_t mipLevel = 0;
    uint32_t arraySlice = 0;
};

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

// 命令缓冲区抽象接口 (现代化版本)
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
    virtual void SetComputePipeline(IComputePipeline* pipeline) = 0;
    virtual void SetViewport(const Viewport& viewport) = 0;
    virtual void SetScissorRect(const Rect& rect) = 0;

    // === 资源绑定 (核心重构) ===
    virtual void SetVertexBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset = 0) = 0;
    virtual void SetIndexBuffer(IBuffer* buffer, bool is32Bit = true, uint32_t offset = 0) = 0;

    /* 绑定描述符集 (Vulkan Set / DX12 Table) */
    virtual void BindDescriptorSet(uint32_t set, IDescriptorSet* descriptorSet) = 0;

    // 推流常量 (用于频繁更新的变换矩阵)
    virtual void PushConstants(ShaderType stage, const void* data, uint32_t size) = 0;

    // === 绘制命令 ===
    virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0) = 0;
    virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0) = 0;

    // === 间接绘制与计算 ===
    virtual void Dispatch(uint32_t x, uint32_t y, uint32_t z) = 0;
    virtual void DrawIndexedIndirect(IBuffer* indirectBuffer, uint32_t offset = 0) = 0;

    // === 资源同步与屏障 ===
    virtual void PipelineBarrier() = 0;
    virtual void PipelineBarrier(const std::vector<ImageBarrier>& imageBarriers) = 0;

    // === 调试 ===
    virtual void BeginDebugGroup(const std::string& name) = 0;
    virtual void EndDebugGroup() = 0;

    // === 原生句柄（后端特定：Vulkan 返回 VkCommandBuffer） ===
    virtual void* GetNativeHandle() const = 0;
};

} // namespace Prisma::Graphic
