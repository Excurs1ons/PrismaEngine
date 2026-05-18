#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>

namespace Prisma::Graphic {
    class IShader;
    class IPipelineState;
    class IRenderTarget;
    class IBuffer;
    class ITexture;
    class ICommandBuffer;
    class ISampler;
    class IDescriptorSet;
    class IDescriptorSetLayout;
}

namespace Prisma::Scripting {

// Handle types
using ShaderHandle = uint32_t;
using PipelineHandle = uint32_t;
using RenderTargetHandle = uint32_t;
using DepthTargetHandle = uint32_t;
using BufferHandle = uint32_t;
using TextureHandle = uint32_t;
using SamplerHandle = uint32_t;

// Sampler desc - binary compatible with C#
struct SRPSamplerDesc {
    uint32_t minFilter;   // 0=Nearest, 1=Linear
    uint32_t magFilter;
    uint32_t mipFilter;
    uint32_t addressU;    // 0=Wrap, 1=Mirror, 2=Clamp, 3=Border, 4=MirrorOnce
    uint32_t addressV;
    uint32_t addressW;
};

// Pipeline creation desc - binary compatible with C#
struct SRPPipelineDesc {
    uint32_t vertexShader;
    uint32_t fragmentShader;
    uint32_t numRenderTargets;
    uint32_t renderTargetFormats[8];
    uint32_t depthStencilFormat;
    uint32_t sampleCount;
    uint32_t cullMode;
    uint8_t  depthTest;
    uint8_t  depthWrite;
    uint8_t  depthFunc;
    uint8_t  blendEnable;
    uint8_t  blendColorWriteMask;
    float    clearColor[4];
    uint32_t topology;  // 0=PointList,1=LineList,2=LineStrip,3=TriangleList,4=TriangleStrip (PrimitiveTopology)
    uint8_t  padding[4]; // align to 8 bytes
};

// Shader stage
enum class SRPShaderStage : uint32_t {
    Vertex = 0, Fragment = 1, Compute = 2, Geometry = 3
};

// ============================================================================
// SRP Graphics API
// ============================================================================
class SRPGraphicsAPI {
public:
    static SRPGraphicsAPI& Get();

    ShaderHandle CreateShader(const char* source, uint32_t sourceLen, SRPShaderStage stage);
    void DestroyShader(ShaderHandle h);
    std::shared_ptr<Graphic::IShader> GetShaderPtr(ShaderHandle h);

    PipelineHandle CreatePipeline(const SRPPipelineDesc& desc);
    void DestroyPipeline(PipelineHandle h);

    RenderTargetHandle CreateRenderTarget(int w, int h, uint32_t format, int samples);
    DepthTargetHandle CreateDepthTarget(int w, int h, uint32_t format);
    void DestroyRenderTarget(RenderTargetHandle h);
    void DestroyDepthTarget(DepthTargetHandle h);

    BufferHandle CreateVertexBuffer(const void* data, uint32_t size, uint32_t stride);
    BufferHandle CreateIndexBuffer(const void* data, uint32_t size, bool is32Bit);
    void DestroyBuffer(BufferHandle h);

    TextureHandle CreateTexture2D(int w, int h, uint32_t format, const void* pixels, uint32_t pixelSize);
    void DestroyTexture(TextureHandle h);

    // Frame lifecycle - uses device's current command buffer
    void BeginFrame();
    void EndFrame();

    // Command recording (between BeginFrame/EndFrame)
    void CmdBeginRenderPass(uint32_t rtCount, const uint32_t* rtHandles, uint32_t depthHandle,
                            const float* clearColors, float depthClear, int viewW, int viewH);
    void CmdEndRenderPass();
    void CmdBindPipeline(PipelineHandle h);
    void CmdBindVertexBuffer(BufferHandle h, uint32_t slot, uint32_t offset);
    void CmdBindIndexBuffer(BufferHandle h, uint32_t offset, int is32Bit);
    void CmdSetViewport(int x, int y, int w, int h);
    void CmdSetScissor(int x, int y, int w, int h);
    void CmdPushConstants(uint32_t offset, uint32_t size, const void* data);
    void CmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex);
    void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset);
    void CmdDrawFullScreenQuad();

    // Texture & sampler
    std::shared_ptr<Graphic::ITexture> GetTexturePtr(TextureHandle h);
    SamplerHandle CreateSampler(const SRPSamplerDesc& desc);
    void DestroySampler(SamplerHandle h);
    std::shared_ptr<Graphic::ISampler> GetSamplerPtr(SamplerHandle h);
    void CmdBindTexture(uint32_t slot, TextureHandle tex, SamplerHandle sampler);

    void Shutdown();

private:
    SRPGraphicsAPI() = default;

    std::vector<std::shared_ptr<Graphic::IShader>> m_shaders;
    std::vector<std::shared_ptr<Graphic::IPipelineState>> m_pipelines;
    std::vector<std::shared_ptr<Graphic::IRenderTarget>> m_renderTargets;
    std::vector<std::shared_ptr<Graphic::ITexture>> m_depthTargets;
    std::vector<std::shared_ptr<Graphic::IBuffer>> m_buffers;
    std::vector<std::shared_ptr<Graphic::ITexture>> m_textures;
    std::vector<std::shared_ptr<Graphic::ISampler>> m_samplers;
    std::unordered_map<PipelineHandle, std::shared_ptr<Graphic::IDescriptorSet>> m_frameDescriptorSets;
    PipelineHandle m_currentPipeline = 0;

    // Current frame command buffer (not owned - from device)
    Graphic::ICommandBuffer* m_cmdBuffer = nullptr;
};

// C-callable wrappers for PrismaAPI
extern "C" {
    uint32_t SRP_CreateShader(const char* s, uint32_t len, uint32_t stage);
    void SRP_DestroyShader(uint32_t h);
    uint32_t SRP_CreatePipeline(const SRPPipelineDesc* d);
    void SRP_DestroyPipeline(uint32_t h);
    uint32_t SRP_CreateRenderTarget(int w, int h, uint32_t f, int s);
    uint32_t SRP_CreateDepthTarget(int w, int h, uint32_t f);
    void SRP_DestroyRenderTarget(uint32_t h);
    void SRP_DestroyDepthTarget(uint32_t h);
    uint32_t SRP_CreateVertexBuffer(const void* d, uint32_t sz, uint32_t st);
    uint32_t SRP_CreateIndexBuffer(const void* d, uint32_t sz, int is32);
    void SRP_DestroyBuffer(uint32_t h);
    uint32_t SRP_CreateTexture2D(int w, int h, uint32_t f, const void* p, uint32_t ps);
    void SRP_DestroyTexture(uint32_t h);
    uint32_t SRP_CreateSampler(const SRPSamplerDesc* d);
    void SRP_DestroySampler(uint32_t h);
    void SRP_CmdBindTexture(uint32_t slot, uint32_t tex, uint32_t sampler);
    void SRP_BeginFrame();
    void SRP_EndFrame();
    void SRP_CmdBeginRenderPass(uint32_t rc, const uint32_t* rh, uint32_t dh, const float* cc, float dc, int vw, int vh);
    void SRP_CmdEndRenderPass();
    void SRP_CmdBindPipeline(uint32_t h);
    void SRP_CmdBindVertexBuffer(uint32_t h, uint32_t s, uint32_t o);
    void SRP_CmdBindIndexBuffer(uint32_t h, uint32_t o, int is32);
    void SRP_CmdSetViewport(int x, int y, int w, int h);
    void SRP_CmdSetScissor(int x, int y, int w, int h);
    void SRP_CmdPushConstants(uint32_t o, uint32_t sz, const void* d);
    void SRP_CmdDraw(uint32_t vc, uint32_t ic, uint32_t fv);
    void SRP_CmdDrawIndexed(uint32_t ic, uint32_t instc, uint32_t fi, int32_t vo);
    void SRP_CmdDrawFullScreenQuad();
    void SRP_Shutdown();
}

} // namespace Prisma::Scripting
