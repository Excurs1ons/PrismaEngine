#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace Prisma::Graphic {
    class IShader;
    class IPipelineState;
    class IRenderTarget;
    class IBuffer;
    class ITexture;

}

namespace Prisma::Scripting {

// ============================================================================
// SRP Graphics API - Thin C wrapper around engine rendering for C# interop
// ============================================================================

// Handle types (uint32_t, 0 = invalid)
using ShaderHandle = uint32_t;
using PipelineHandle = uint32_t;
using RenderTargetHandle = uint32_t;
using DepthTargetHandle = uint32_t;
using BufferHandle = uint32_t;
using TextureHandle = uint32_t;

// C-compatible struct for pipeline creation from C#
struct SRPPipelineDesc {
    ShaderHandle vertexShader;
    ShaderHandle fragmentShader;
    uint32_t numRenderTargets;
    uint32_t renderTargetFormats[8];  // TextureFormat enum values
    uint32_t depthStencilFormat;       // TextureFormat enum value
    uint32_t sampleCount;
    uint32_t cullMode;                 // 0=none, 1=front, 2=back
    bool depthTest;
    bool depthWrite;
    uint8_t depthFunc;                 // 4=less (D3D12 default)
    bool blendEnable;
    uint8_t blendColorWriteMask;       // 0xF = RGBA
    float clearColor[4];
};

// Shader stage enum
enum class SRPShaderStage : uint32_t {
    Vertex = 0,
    Fragment = 1,
    Compute = 2,
    Geometry = 3
};

// ============================================================================
// SRP Graphics API - singleton controller
// ============================================================================
class SRPGraphicsAPI {
public:
    static SRPGraphicsAPI& Get();

    // --- Shader ---
    ShaderHandle CreateShader(const char* source, uint32_t sourceLen, SRPShaderStage stage);
    void DestroyShader(ShaderHandle handle);
    std::shared_ptr<Graphic::IShader> GetShaderPtr(ShaderHandle handle);

    // --- Pipeline ---
    PipelineHandle CreatePipeline(const SRPPipelineDesc& desc);
    void DestroyPipeline(PipelineHandle handle);
    Graphic::IPipelineState* GetPipeline(PipelineHandle handle);

    // --- Render Target ---
    RenderTargetHandle CreateRenderTarget(int width, int height, uint32_t format, int samples);
    DepthTargetHandle CreateDepthTarget(int width, int height, uint32_t format);
    void DestroyRenderTarget(RenderTargetHandle handle);
    void DestroyDepthTarget(DepthTargetHandle handle);
    Graphic::IRenderTarget* GetRenderTarget(RenderTargetHandle handle);
    Graphic::ITexture* GetDepthTarget(DepthTargetHandle handle);

    // --- Vertex/Index Buffer ---
    BufferHandle CreateVertexBuffer(const void* data, uint32_t size, uint32_t stride);
    BufferHandle CreateIndexBuffer(const void* data, uint32_t size, bool is32Bit);
    void DestroyBuffer(BufferHandle handle);
    Graphic::IBuffer* GetBuffer(BufferHandle handle);

    // --- Texture (2D) ---
    TextureHandle CreateTexture2D(int width, int height, uint32_t format, const void* pixels, uint32_t pixelSize);
    void DestroyTexture(TextureHandle handle);
    Graphic::ITexture* GetTexture(TextureHandle handle);

    // --- Per-frame state ---
    void BeginFrame();
    void EndFrame();

    // --- Command Buffer Methods (called between BeginFrame/EndFrame) ---
    void CmdBeginRenderPass(uint32_t rtCount, const uint32_t* rtHandles, uint32_t depthHandle, const float* clearColors, float depthClear, int viewW, int viewH);
    void CmdEndRenderPass();
    void CmdBindPipeline(PipelineHandle handle);
    void CmdBindVertexBuffer(BufferHandle handle, uint32_t slot, uint32_t offset);
    void CmdBindIndexBuffer(BufferHandle handle, uint32_t offset, int is32Bit);
    void CmdSetViewport(int x, int y, int w, int h);
    void CmdSetScissor(int x, int y, int w, int h);
    void CmdPushConstants(uint32_t offset, uint32_t size, const void* data);
    void CmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex);
    void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset);
    void CmdDrawFullScreenQuad();

    // --- Cleanup ---
    void Shutdown();

private:
    SRPGraphicsAPI() = default;

    std::vector<std::shared_ptr<Graphic::IShader>> m_shaders;
    std::vector<std::shared_ptr<Graphic::IPipelineState>> m_pipelines;
    std::vector<std::shared_ptr<Graphic::IRenderTarget>> m_renderTargets;
    std::vector<std::shared_ptr<Graphic::ITexture>> m_depthTargets;
    std::vector<std::shared_ptr<Graphic::IBuffer>> m_buffers;
    std::vector<std::shared_ptr<Graphic::ITexture>> m_textures;
};

// ============================================================================
// C-callable function pointers for PrismaAPI
// ============================================================================
extern "C" {

// Shader
uint32_t SRP_CreateShader(const char* source, uint32_t sourceLen, uint32_t stage);
void SRP_DestroyShader(uint32_t handle);

// Pipeline
uint32_t SRP_CreatePipeline(const SRPPipelineDesc* desc);
void SRP_DestroyPipeline(uint32_t handle);

// Render Target
uint32_t SRP_CreateRenderTarget(int w, int h, uint32_t format, int samples);
uint32_t SRP_CreateDepthTarget(int w, int h, uint32_t format);
void SRP_DestroyRenderTarget(uint32_t handle);
void SRP_DestroyDepthTarget(uint32_t handle);

// Buffers
uint32_t SRP_CreateVertexBuffer(const void* data, uint32_t size, uint32_t stride);
uint32_t SRP_CreateIndexBuffer(const void* data, uint32_t size, int is32Bit);
void SRP_DestroyBuffer(uint32_t handle);

// Texture
uint32_t SRP_CreateTexture2D(int w, int h, uint32_t format, const void* pixels, uint32_t pixelSize);
void SRP_DestroyTexture(uint32_t handle);

// Frame commands
void SRP_BeginFrame();
void SRP_EndFrame();

// Command buffer (called between BeginFrame/EndFrame)
void SRP_CmdBeginRenderPass(uint32_t rtCount, const uint32_t* rtHandles, uint32_t depthHandle, const float* clearColors, float depthClear, int viewW, int viewH);
void SRP_CmdEndRenderPass();
void SRP_CmdBindPipeline(uint32_t handle);
void SRP_CmdBindVertexBuffer(uint32_t handle, uint32_t slot, uint32_t offset);
void SRP_CmdBindIndexBuffer(uint32_t handle, uint32_t offset, int is32Bit);
void SRP_CmdSetViewport(int x, int y, int w, int h);
void SRP_CmdSetScissor(int x, int y, int w, int h);
void SRP_CmdPushConstants(uint32_t offset, uint32_t size, const void* data);
void SRP_CmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex);
void SRP_CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset);
void SRP_CmdDrawFullScreenQuad();

// Cleanup
void SRP_Shutdown();

} // extern "C"

} // namespace Prisma::Scripting
