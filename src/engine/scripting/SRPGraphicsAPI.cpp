#include "SRPGraphicsAPI.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/ShaderFactory.h"
#include "graphic/RenderDesc.h"
#include "adapters/vulkan/RenderDeviceVulkan.h"
#include "Logger.h"

namespace Prisma::Scripting {
namespace G = Prisma::Graphic;
namespace Vk = Prisma::Graphic::Vulkan;

static G::IRenderDevice* GetDev() {
    auto* rs = Prisma::Engine::Get().GetRenderSystem();
    return rs ? rs->GetDevice() : nullptr;
}
static G::IResourceFactory* GetFac() {
    auto* d = GetDev(); return d ? d->GetResourceFactory() : nullptr;
}

SRPGraphicsAPI& SRPGraphicsAPI::Get() {
    static SRPGraphicsAPI inst; return inst;
}

// === Shader ===

ShaderHandle SRPGraphicsAPI::CreateShader(const char* src, uint32_t len, SRPShaderStage stage) {
    auto* dev = GetDev(); if (!dev) return 0;
    G::ShaderType t = G::ShaderType::Vertex;
    switch (stage) {
        case SRPShaderStage::Vertex:   t = G::ShaderType::Vertex; break;
        case SRPShaderStage::Fragment: t = G::ShaderType::Pixel; break;
        case SRPShaderStage::Compute:  t = G::ShaderType::Compute; break;
        case SRPShaderStage::Geometry: t = G::ShaderType::Geometry; break;
    }
    G::ShaderDesc sd; sd.type = t; sd.language = G::ShaderLanguage::GLSL;
    sd.source.assign(src, len); sd.filename = "srp";
    auto sh = G::ShaderFactory::CreateShader(dev, sd); if (!sh) return 0;
    ShaderHandle h = (ShaderHandle)(m_shaders.size() + 1);
    m_shaders.push_back(sh); return h;
}

void SRPGraphicsAPI::DestroyShader(ShaderHandle h) {
    if (h > 0 && h - 1 < m_shaders.size()) m_shaders[h - 1].reset();
}

std::shared_ptr<G::IShader> SRPGraphicsAPI::GetShaderPtr(ShaderHandle h) {
    return (h > 0 && h - 1 < m_shaders.size()) ? m_shaders[h - 1] : nullptr;
}

// === Pipeline ===

PipelineHandle SRPGraphicsAPI::CreatePipeline(const SRPPipelineDesc& d) {
    auto* dev = GetDev(); auto* fac = GetFac();
    if (!dev || !fac) return 0;
    auto vs = GetShaderPtr(d.vertexShader); auto fs = GetShaderPtr(d.fragmentShader);
    if (!vs || !fs) return 0;

    auto pso = fac->CreatePipelineStateImpl(); if (!pso) return 0;
    pso->SetShader(G::ShaderType::Vertex, vs);
    pso->SetShader(G::ShaderType::Pixel, fs);
    pso->SetPrimitiveTopology(G::PrimitiveTopology::TriangleList);
    pso->SetSampleCount(d.sampleCount);
    G::BlendState bs; bs.blendEnable = d.blendEnable != 0; bs.writeMask = d.blendColorWriteMask;
    pso->SetBlendState(bs);
    G::RasterizerState rs; rs.cullEnable = d.cullMode > 0; rs.cullMode = (G::CullMode)d.cullMode;
    pso->SetRasterizerState(rs);
    G::DepthStencilState ds; ds.depthEnable = d.depthTest != 0; ds.depthWriteEnable = d.depthWrite != 0; ds.depthFunc = (G::ComparisonFunc)d.depthFunc;
    pso->SetDepthStencilState(ds);
    std::vector<G::TextureFormat> rtf;
    for (uint32_t i = 0; i < d.numRenderTargets && i < 8; i++) rtf.push_back((G::TextureFormat)d.renderTargetFormats[i]);
    pso->SetRenderTargetFormats(rtf);
    pso->SetDepthStencilFormat((G::TextureFormat)d.depthStencilFormat);
    if (!pso->Create(dev)) return 0;
    PipelineHandle h = (PipelineHandle)(m_pipelines.size() + 1);
    m_pipelines.push_back(std::move(pso)); return h;
}

void SRPGraphicsAPI::DestroyPipeline(PipelineHandle h) {
    if (h > 0 && h - 1 < m_pipelines.size()) m_pipelines[h - 1].reset();
}

// === Render Target ===

RenderTargetHandle SRPGraphicsAPI::CreateRenderTarget(int, int, uint32_t, int) { return 0; }

DepthTargetHandle SRPGraphicsAPI::CreateDepthTarget(int w, int h, uint32_t fmt) {
    auto* fac = GetFac(); if (!fac) return 0;
    G::TextureDesc td; td.type = G::TextureType::Texture2D; td.format = (G::TextureFormat)fmt;
    td.width = (uint32_t)w; td.height = (uint32_t)h; td.allowRenderTarget = true; td.allowDepthStencil = true;
    auto tex = fac->CreateTextureImpl(td); if (!tex) return 0;
    DepthTargetHandle hdl = (DepthTargetHandle)(m_depthTargets.size() + 1);
    m_depthTargets.push_back(std::move(tex)); return hdl;
}

void SRPGraphicsAPI::DestroyRenderTarget(RenderTargetHandle h) {
    if (h > 0 && h - 1 < m_renderTargets.size()) m_renderTargets[h - 1].reset();
}
void SRPGraphicsAPI::DestroyDepthTarget(DepthTargetHandle h) {
    if (h > 0 && h - 1 < m_depthTargets.size()) m_depthTargets[h - 1].reset();
}

// === Buffers ===

BufferHandle SRPGraphicsAPI::CreateVertexBuffer(const void* data, uint32_t size, uint32_t stride) {
    auto* fac = GetFac(); if (!fac) return 0;
    G::BufferDesc bd; bd.type = G::BufferType::Vertex; bd.size = size; bd.stride = stride; bd.initialData = data; bd.usage = G::BufferUsage::Immutable;
    auto buf = fac->CreateBufferImpl(bd); if (!buf) return 0;
    BufferHandle h = (BufferHandle)(m_buffers.size() + 1); m_buffers.push_back(std::move(buf)); return h;
}

BufferHandle SRPGraphicsAPI::CreateIndexBuffer(const void* data, uint32_t size, bool is32) {
    auto* fac = GetFac(); if (!fac) return 0;
    G::BufferDesc bd; bd.type = G::BufferType::Index; bd.size = size; bd.stride = is32 ? 4 : 2; bd.initialData = data; bd.usage = G::BufferUsage::Immutable;
    auto buf = fac->CreateBufferImpl(bd); if (!buf) return 0;
    BufferHandle h = (BufferHandle)(m_buffers.size() + 1); m_buffers.push_back(std::move(buf)); return h;
}

void SRPGraphicsAPI::DestroyBuffer(BufferHandle h) {
    if (h > 0 && h - 1 < m_buffers.size()) m_buffers[h - 1].reset();
}

// === Texture (stub) ===

TextureHandle SRPGraphicsAPI::CreateTexture2D(int, int, uint32_t, const void*, uint32_t) { return 0; }
void SRPGraphicsAPI::DestroyTexture(TextureHandle) {}

// === Frame & Command Recording ===

void SRPGraphicsAPI::BeginFrame() {
    auto* dev = GetDev();
    if (!dev) { m_cmdBuffer = nullptr; return; }
    // Get device's current command buffer (recording after BeginFrame)
    auto* vkDev = dynamic_cast<Vk::RenderDeviceVulkan*>(dev);
    if (vkDev) {
        vkDev->SuspendDefaultRenderPass();
        m_cmdBuffer = vkDev->GetCurrentCommandBuffer();
    }
}

void SRPGraphicsAPI::EndFrame() {
    auto* dev = dynamic_cast<Vk::RenderDeviceVulkan*>(GetDev());
    if (dev && m_cmdBuffer) {
        dev->ResumeDefaultRenderPass();
    }
    m_cmdBuffer = nullptr;
}

void SRPGraphicsAPI::CmdBeginRenderPass(uint32_t, const uint32_t*, uint32_t, const float*, float, int, int) {
    if (!m_cmdBuffer) return;
    G::RenderPassDesc desc;
    desc.renderTarget = nullptr; // TODO: lookup RT handle
    desc.clearRenderTarget = false;
    desc.renderArea = {0, 0, 0, 0};
    m_cmdBuffer->BeginRenderPass(desc);
}

void SRPGraphicsAPI::CmdEndRenderPass() { if (m_cmdBuffer) m_cmdBuffer->EndRenderPass(); }
void SRPGraphicsAPI::CmdBindPipeline(PipelineHandle h) {
    if (!m_cmdBuffer || !(h > 0 && h - 1 < m_pipelines.size())) return;
    m_cmdBuffer->SetPipelineState(m_pipelines[h - 1].get());
}
void SRPGraphicsAPI::CmdBindVertexBuffer(BufferHandle h, uint32_t s, uint32_t o) {
    if (!m_cmdBuffer || !(h > 0 && h - 1 < m_buffers.size())) return;
    m_cmdBuffer->SetVertexBuffer(m_buffers[h - 1].get(), s, o);
}
void SRPGraphicsAPI::CmdBindIndexBuffer(BufferHandle h, uint32_t o, int is32) {
    if (!m_cmdBuffer || !(h > 0 && h - 1 < m_buffers.size())) return;
    m_cmdBuffer->SetIndexBuffer(m_buffers[h - 1].get(), is32 != 0, o);
}
void SRPGraphicsAPI::CmdSetViewport(int x, int y, int w, int h) {
    if (!m_cmdBuffer) return;
    G::Viewport vp; vp.x = (float)x; vp.y = (float)y; vp.width = (float)w; vp.height = (float)h; vp.minDepth = 0; vp.maxDepth = 1;
    m_cmdBuffer->SetViewport(vp);
}
void SRPGraphicsAPI::CmdSetScissor(int x, int y, int w, int h) {
    if (!m_cmdBuffer) return;
    G::Rect r; r.x = x; r.y = y; r.width = w; r.height = h;
    m_cmdBuffer->SetScissorRect(r);
}
void SRPGraphicsAPI::CmdPushConstants(uint32_t o, uint32_t sz, const void* d) { if (m_cmdBuffer) m_cmdBuffer->PushConstants(G::ShaderType::Vertex, d, sz); }
void SRPGraphicsAPI::CmdDraw(uint32_t vc, uint32_t ic, uint32_t fv) { if (m_cmdBuffer) m_cmdBuffer->Draw(vc, ic, fv); }
void SRPGraphicsAPI::CmdDrawIndexed(uint32_t ic, uint32_t instc, uint32_t fi, int32_t vo) { if (m_cmdBuffer) m_cmdBuffer->DrawIndexed(ic, instc, fi, vo); }
void SRPGraphicsAPI::CmdDrawFullScreenQuad() { if (m_cmdBuffer) m_cmdBuffer->Draw(4, 1, 0); }

void SRPGraphicsAPI::Shutdown() {
    m_shaders.clear(); m_pipelines.clear(); m_renderTargets.clear();
    m_depthTargets.clear(); m_buffers.clear(); m_textures.clear();
    m_cmdBuffer = nullptr;
}

// ===== C Wrappers =====

#define C0(name) void SRP_##name() { SRPGraphicsAPI::Get().name(); }

uint32_t SRP_CreateShader(const char* s, uint32_t l, uint32_t st) { return SRPGraphicsAPI::Get().CreateShader(s, l, (SRPShaderStage)st); }
void SRP_DestroyShader(uint32_t h) { SRPGraphicsAPI::Get().DestroyShader(h); }
uint32_t SRP_CreatePipeline(const SRPPipelineDesc* d) { return d ? SRPGraphicsAPI::Get().CreatePipeline(*d) : 0; }
void SRP_DestroyPipeline(uint32_t h) { SRPGraphicsAPI::Get().DestroyPipeline(h); }
uint32_t SRP_CreateRenderTarget(int w, int h, uint32_t f, int s) { return SRPGraphicsAPI::Get().CreateRenderTarget(w, h, f, s); }
uint32_t SRP_CreateDepthTarget(int w, int h, uint32_t f) { return SRPGraphicsAPI::Get().CreateDepthTarget(w, h, f); }
void SRP_DestroyRenderTarget(uint32_t h) { SRPGraphicsAPI::Get().DestroyRenderTarget(h); }
void SRP_DestroyDepthTarget(uint32_t h) { SRPGraphicsAPI::Get().DestroyDepthTarget(h); }
uint32_t SRP_CreateVertexBuffer(const void* d, uint32_t sz, uint32_t st) { return SRPGraphicsAPI::Get().CreateVertexBuffer(d, sz, st); }
uint32_t SRP_CreateIndexBuffer(const void* d, uint32_t sz, int is32) { return SRPGraphicsAPI::Get().CreateIndexBuffer(d, sz, is32 != 0); }
void SRP_DestroyBuffer(uint32_t h) { SRPGraphicsAPI::Get().DestroyBuffer(h); }
uint32_t SRP_CreateTexture2D(int w, int h, uint32_t f, const void* p, uint32_t ps) { return SRPGraphicsAPI::Get().CreateTexture2D(w, h, f, p, ps); }
void SRP_DestroyTexture(uint32_t h) { SRPGraphicsAPI::Get().DestroyTexture(h); }
void SRP_BeginFrame() { SRPGraphicsAPI::Get().BeginFrame(); }
void SRP_EndFrame() { SRPGraphicsAPI::Get().EndFrame(); }
void SRP_CmdBeginRenderPass(uint32_t rc, const uint32_t* rh, uint32_t dh, const float* cc, float dc, int vw, int vh) { SRPGraphicsAPI::Get().CmdBeginRenderPass(rc, rh, dh, cc, dc, vw, vh); }
C0(CmdEndRenderPass)
void SRP_CmdBindPipeline(uint32_t h) { SRPGraphicsAPI::Get().CmdBindPipeline(h); }
void SRP_CmdBindVertexBuffer(uint32_t h, uint32_t s, uint32_t o) { SRPGraphicsAPI::Get().CmdBindVertexBuffer(h, s, o); }
void SRP_CmdBindIndexBuffer(uint32_t h, uint32_t o, int is32) { SRPGraphicsAPI::Get().CmdBindIndexBuffer(h, o, is32); }
void SRP_CmdSetViewport(int x, int y, int w, int h) { SRPGraphicsAPI::Get().CmdSetViewport(x, y, w, h); }
void SRP_CmdSetScissor(int x, int y, int w, int h) { SRPGraphicsAPI::Get().CmdSetScissor(x, y, w, h); }
void SRP_CmdPushConstants(uint32_t o, uint32_t sz, const void* d) { SRPGraphicsAPI::Get().CmdPushConstants(o, sz, d); }
void SRP_CmdDraw(uint32_t vc, uint32_t ic, uint32_t fv) { SRPGraphicsAPI::Get().CmdDraw(vc, ic, fv); }
void SRP_CmdDrawIndexed(uint32_t ic, uint32_t instc, uint32_t fi, int32_t vo) { SRPGraphicsAPI::Get().CmdDrawIndexed(ic, instc, fi, vo); }
void SRP_CmdDrawFullScreenQuad() { SRPGraphicsAPI::Get().CmdDrawFullScreenQuad(); }
void SRP_Shutdown() { SRPGraphicsAPI::Get().Shutdown(); }

} // namespace Prisma::Scripting
