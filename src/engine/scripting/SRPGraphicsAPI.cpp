#include "SRPGraphicsAPI.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/ShaderFactory.h"
#include "graphic/RenderDesc.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "Logger.h"

namespace Prisma::Scripting {
namespace G = Prisma::Graphic;

static G::IRenderDevice* GetDevice() {
    auto* rs = Prisma::Engine::Get().GetRenderSystem();
    return rs ? rs->GetDevice() : nullptr;
}
static G::IResourceFactory* GetFactory() {
    auto* dev = GetDevice();
    return dev ? dev->GetResourceFactory() : nullptr;
}

SRPGraphicsAPI& SRPGraphicsAPI::Get() {
    static SRPGraphicsAPI instance;
    return instance;
}

// === Shader ===

ShaderHandle SRPGraphicsAPI::CreateShader(const char* source, uint32_t sourceLen, SRPShaderStage stage) {
    auto* device = GetDevice();
    if (!device) return 0;
    G::ShaderType t = G::ShaderType::Vertex;
    switch (stage) {
        case SRPShaderStage::Vertex:   t = G::ShaderType::Vertex; break;
        case SRPShaderStage::Fragment: t = G::ShaderType::Pixel; break;
        case SRPShaderStage::Compute:  t = G::ShaderType::Compute; break;
        case SRPShaderStage::Geometry: t = G::ShaderType::Geometry; break;
    }
    G::ShaderDesc desc;
    desc.type = t;
    desc.language = G::ShaderLanguage::GLSL;
    desc.source.assign(source, sourceLen);
    desc.filename = "srp_shader";
    auto shader = G::ShaderFactory::CreateShader(device, desc);
    if (!shader) return 0;
    ShaderHandle h = (ShaderHandle)(m_shaders.size() + 1);
    m_shaders.push_back(shader);
    return h;
}

void SRPGraphicsAPI::DestroyShader(ShaderHandle h) {
    if (h > 0 && h - 1 < m_shaders.size()) m_shaders[h - 1].reset();
}

std::shared_ptr<G::IShader> SRPGraphicsAPI::GetShaderPtr(ShaderHandle h) {
    if (h > 0 && h - 1 < m_shaders.size()) return m_shaders[h - 1];
    return nullptr;
}

// === Pipeline ===

PipelineHandle SRPGraphicsAPI::CreatePipeline(const SRPPipelineDesc& d) {
    auto* dev = GetDevice();
    auto* fac = GetFactory();
    if (!dev || !fac) return 0;
    auto vs = GetShaderPtr(d.vertexShader);
    auto fs = GetShaderPtr(d.fragmentShader);
    if (!vs || !fs) return 0;
    auto pso = fac->CreatePipelineStateImpl();
    if (!pso) return 0;
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
    m_pipelines.push_back(std::move(pso));
    return h;
}

void SRPGraphicsAPI::DestroyPipeline(PipelineHandle h) {
    if (h > 0 && h - 1 < m_pipelines.size()) m_pipelines[h - 1].reset();
}

// === Render Target (TODO) ===

RenderTargetHandle SRPGraphicsAPI::CreateRenderTarget(int, int, uint32_t, int) { return 0; }
DepthTargetHandle SRPGraphicsAPI::CreateDepthTarget(int w, int h, uint32_t format) {
    auto* fac = GetFactory(); if (!fac) return 0;
    G::TextureDesc td;
    td.type = G::TextureType::Texture2D;
    td.format = static_cast<G::TextureFormat>(format);
    td.width = (uint32_t)w; td.height = (uint32_t)h;
    td.allowRenderTarget = true;
    td.allowDepthStencil = true;
    auto tex = fac->CreateTextureImpl(td);
    if (!tex) return 0;
    DepthTargetHandle hdl = (DepthTargetHandle)(m_depthTargets.size() + 1);
    m_depthTargets.push_back(std::shared_ptr<G::ITexture>(std::move(tex)));
    return hdl;
}
void SRPGraphicsAPI::DestroyRenderTarget(RenderTargetHandle) {}
void SRPGraphicsAPI::DestroyDepthTarget(DepthTargetHandle) {}

// === Buffers ===

BufferHandle SRPGraphicsAPI::CreateVertexBuffer(const void* data, uint32_t size, uint32_t stride) {
    auto* fac = GetFactory(); if (!fac) return 0;
    G::BufferDesc bd; bd.type = G::BufferType::Vertex; bd.size = size; bd.stride = stride; bd.initialData = data; bd.usage = G::BufferUsage::Immutable;
    auto buf = fac->CreateBufferImpl(bd); if (!buf) return 0;
    BufferHandle h = (BufferHandle)(m_buffers.size() + 1); m_buffers.push_back(std::move(buf)); return h;
}

BufferHandle SRPGraphicsAPI::CreateIndexBuffer(const void* data, uint32_t size, bool is32Bit) {
    auto* fac = GetFactory(); if (!fac) return 0;
    G::BufferDesc bd; bd.type = G::BufferType::Index; bd.size = size; bd.stride = is32Bit ? 4 : 2; bd.initialData = data; bd.usage = G::BufferUsage::Immutable;
    auto buf = fac->CreateBufferImpl(bd); if (!buf) return 0;
    BufferHandle h = (BufferHandle)(m_buffers.size() + 1); m_buffers.push_back(std::move(buf)); return h;
}

void SRPGraphicsAPI::DestroyBuffer(BufferHandle h) {
    if (h > 0 && h - 1 < m_buffers.size()) m_buffers[h - 1].reset();
}

// === Texture (TODO) ===

TextureHandle SRPGraphicsAPI::CreateTexture2D(int, int, uint32_t, const void*, uint32_t) { return 0; }
void SRPGraphicsAPI::DestroyTexture(TextureHandle) {}

// === Frame ===
void SRPGraphicsAPI::BeginFrame() {}
void SRPGraphicsAPI::EndFrame() {}

// === Command Buffer (stubs - will use srpRender callback instead) ===
void SRPGraphicsAPI::CmdBeginRenderPass(uint32_t, const uint32_t*, uint32_t, const float*, float, int, int) {}
void SRPGraphicsAPI::CmdEndRenderPass() {}
void SRPGraphicsAPI::CmdBindPipeline(PipelineHandle) {}
void SRPGraphicsAPI::CmdBindVertexBuffer(BufferHandle, uint32_t, uint32_t) {}
void SRPGraphicsAPI::CmdBindIndexBuffer(BufferHandle, uint32_t, int) {}
void SRPGraphicsAPI::CmdSetViewport(int, int, int, int) {}
void SRPGraphicsAPI::CmdSetScissor(int, int, int, int) {}
void SRPGraphicsAPI::CmdPushConstants(uint32_t, uint32_t, const void*) {}
void SRPGraphicsAPI::CmdDraw(uint32_t, uint32_t, uint32_t) {}
void SRPGraphicsAPI::CmdDrawIndexed(uint32_t, uint32_t, uint32_t, int32_t) {}
void SRPGraphicsAPI::CmdDrawFullScreenQuad() {}

void SRPGraphicsAPI::Shutdown() {
    m_shaders.clear(); m_pipelines.clear();
    m_renderTargets.clear(); m_depthTargets.clear();
    m_buffers.clear(); m_textures.clear();
}

// ===== C-callable wrappers =====

uint32_t SRP_CreateShader(const char* s, uint32_t len, uint32_t stage) { return SRPGraphicsAPI::Get().CreateShader(s, len, (SRPShaderStage)stage); }
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
void SRP_CmdEndRenderPass() { SRPGraphicsAPI::Get().CmdEndRenderPass(); }
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
