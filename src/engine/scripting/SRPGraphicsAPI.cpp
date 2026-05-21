#include "SRPGraphicsAPI.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IComputePipeline.h"
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
    // topology: 3=TriangleList, 4=TriangleStrip, else default TriangleList
    pso->SetPrimitiveTopology(d.topology == 4 ? G::PrimitiveTopology::TriangleStrip : G::PrimitiveTopology::TriangleList);
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
    if (!pso->Create(dev)) { LOG_ERROR("SRP", "PSO Create failed: {0}", pso->GetErrors()); return 0; }
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

TextureHandle SRPGraphicsAPI::CreateTexture2D(int w, int h, uint32_t format, const void* data, uint32_t dataSize) {
    auto* fac = GetFac();
    if (!fac || !data || w <= 0 || h <= 0) return 0;
    G::TextureDesc td;
    td.type = G::TextureType::Texture2D;
    td.format = (G::TextureFormat)format;
    td.width = (uint32_t)w;
    td.height = (uint32_t)h;
    td.allowShaderResource = true;
    td.initialData = data;
    td.dataSize = dataSize;
    auto tex = fac->CreateTextureImpl(td);
    if (!tex) return 0;
    TextureHandle hdl = (TextureHandle)(m_textures.size() + 1);
    m_textures.push_back(std::move(tex));
    return hdl;
}

void SRPGraphicsAPI::DestroyTexture(TextureHandle h) {
    if (h > 0 && h - 1 < m_textures.size()) m_textures[h - 1].reset();
}

std::shared_ptr<G::ITexture> SRPGraphicsAPI::GetTexturePtr(TextureHandle h) {
    return (h > 0 && h - 1 < m_textures.size()) ? m_textures[h - 1] : nullptr;
}

// === Frame & Command Recording ===

void SRPGraphicsAPI::BeginFrame() {
    auto* dev = GetDev();
    if (!dev) { m_cmdBuffer = nullptr; return; }
    // Get device's current command buffer (swap chain render pass is already active)
    // SRP draws directly into the swap chain's render pass
    auto* vkDev = dynamic_cast<Vk::RenderDeviceVulkan*>(dev);
    if (vkDev) {
        m_cmdBuffer = vkDev->GetCurrentCommandBuffer();
    }
}

void SRPGraphicsAPI::EndFrame() {
    // Don't resume the render pass - SRP draws into the swap chain's
    // already-active render pass; RenderDeviceVulkan::EndFrame() will end it.
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
    m_currentPipeline = h;
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
void SRPGraphicsAPI::CmdPushConstants(uint32_t o, uint32_t sz, const void* d) {
    LOG_DEBUG("SRP", "PushConstants: offset={}, size={}", o, sz);
    if (m_cmdBuffer) {
        m_cmdBuffer->PushConstants(G::ShaderType::Vertex, d, sz);
        m_cmdBuffer->PushConstants(G::ShaderType::Pixel, d, sz);
    }
}
void SRPGraphicsAPI::CmdDraw(uint32_t vc, uint32_t ic, uint32_t fv) { if (m_cmdBuffer) m_cmdBuffer->Draw(vc, ic, fv); }
void SRPGraphicsAPI::CmdDrawIndexed(uint32_t ic, uint32_t instc, uint32_t fi, int32_t vo) { if (m_cmdBuffer) m_cmdBuffer->DrawIndexed(ic, instc, fi, vo); }
void SRPGraphicsAPI::CmdDrawFullScreenQuad() { if (m_cmdBuffer) m_cmdBuffer->Draw(4, 1, 0); }

// === Sampler ===

SamplerHandle SRPGraphicsAPI::CreateSampler(const SRPSamplerDesc& d) {
    auto* fac = GetFac(); if (!fac) return 0;
    G::SamplerDesc sd;
    sd.filter = (d.minFilter == 1 || d.magFilter == 1) ? G::TextureFilter::Linear : G::TextureFilter::Point;
    auto mapAddr = [](uint32_t a) -> G::TextureAddressMode {
        switch (a) {
            case 0: return G::TextureAddressMode::Wrap;
            case 1: return G::TextureAddressMode::Mirror;
            case 2: return G::TextureAddressMode::Clamp;
            case 3: return G::TextureAddressMode::Border;
            default: return G::TextureAddressMode::MirrorOnce;
        }
    };
    sd.addressU = mapAddr(d.addressU);
    sd.addressV = mapAddr(d.addressV);
    sd.addressW = mapAddr(d.addressW);
    auto sam = fac->CreateSamplerImpl(sd);
    if (!sam) return 0;
    SamplerHandle h = (SamplerHandle)(m_samplers.size() + 1);
    m_samplers.push_back(std::move(sam));
    return h;
}

void SRPGraphicsAPI::DestroySampler(SamplerHandle h) {
    if (h > 0 && h - 1 < m_samplers.size()) m_samplers[h - 1].reset();
}

std::shared_ptr<G::ISampler> SRPGraphicsAPI::GetSamplerPtr(SamplerHandle h) {
    return (h > 0 && h - 1 < m_samplers.size()) ? m_samplers[h - 1] : nullptr;
}

// === Texture binding ===

void SRPGraphicsAPI::CmdBindTexture(uint32_t slot, TextureHandle tex, SamplerHandle sampler) {
    if (!m_cmdBuffer) return;
    auto* fac = GetFac(); if (!fac) return;
    auto texPtr = GetTexturePtr(tex); if (!texPtr) return;
    auto samPtr = GetSamplerPtr(sampler); if (!samPtr) return;

    // Get descriptor set layout from current pipeline
    if (m_currentPipeline == 0) return;
    auto& pso = m_pipelines[m_currentPipeline - 1];
    const auto& layouts = pso->GetDescriptorSetLayouts();
    if (layouts.empty()) return;

    // Cache descriptor set per pipeline (reuse each frame)
    auto it = m_frameDescriptorSets.find(m_currentPipeline);
    std::shared_ptr<G::IDescriptorSet> descSet;
    if (it != m_frameDescriptorSets.end()) {
        descSet = it->second;
    } else {
        descSet = fac->CreateDescriptorSet(layouts[0].get());
        if (!descSet) return;
        m_frameDescriptorSets[m_currentPipeline] = descSet;
    }

    descSet->BindTexture(slot, texPtr.get(), samPtr.get());
    descSet->Update();
    m_cmdBuffer->BindDescriptorSet(0, descSet.get());
}

void SRPGraphicsAPI::CmdBlitRenderTarget(TextureHandle dst) {
    auto* dev = GetDev(); if (!dev) return;
    auto dstTex = GetTexturePtr(dst); if (!dstTex) return;

    auto* vkDev = dynamic_cast<Vk::RenderDeviceVulkan*>(dev);
    if (!vkDev) return;

    auto* swapChain = vkDev->GetSwapChain();
    if (!swapChain) return;

    auto* currentRT = swapChain->GetCurrentRenderTarget();
    if (!currentRT) return;

    dstTex->CopyFrom(currentRT, 0, 0, 0, 0);
}

// === Compute Pipeline ===

ComputePipelineHandle SRPGraphicsAPI::CreateComputePipeline(ShaderHandle shaderH, uint32_t pushConstSize) {
    auto* dev = GetDev(); auto* fac = GetFac();
    if (!dev || !fac) return 0;
    auto shader = GetShaderPtr(shaderH);
    if (!shader || shader->GetShaderType() != G::ShaderType::Compute) return 0;

    auto pipeline = fac->CreateComputePipelineImpl();
    if (!pipeline) return 0;
    pipeline->SetShader(shader);
    if (pushConstSize > 0) {
        pipeline->SetPushConstantRange(pushConstSize);
    }
    if (!pipeline->Create(dev)) {
        LOG_ERROR("SRP", "CreateComputePipeline failed");
        return 0;
    }
    ComputePipelineHandle h = (ComputePipelineHandle)(m_computePipelines.size() + 1);
    m_computePipelines.push_back(std::move(pipeline));
    return h;
}

void SRPGraphicsAPI::DestroyComputePipeline(ComputePipelineHandle h) {
    if (h > 0 && h - 1 < m_computePipelines.size()) {
        m_computePipelines[h - 1].reset();
        m_computeDescriptorSets.erase(h);
    }
}

void SRPGraphicsAPI::CmdBindComputePipeline(ComputePipelineHandle h) {
    if (!m_cmdBuffer || !(h > 0 && h - 1 < m_computePipelines.size())) return;
    m_cmdBuffer->SetComputePipeline(m_computePipelines[h - 1].get());
    m_currentComputePipeline = h;
}

void SRPGraphicsAPI::CmdDispatch(uint32_t x, uint32_t y, uint32_t z) {
    if (m_cmdBuffer) m_cmdBuffer->Dispatch(x, y, z);
}

void SRPGraphicsAPI::CmdBindComputeTexture(uint32_t slot, TextureHandle tex, SamplerHandle sampler) {
    if (!m_cmdBuffer || m_currentComputePipeline == 0) return;
    auto* fac = GetFac(); if (!fac) return;
    auto texPtr = GetTexturePtr(tex); if (!texPtr) return;
    auto samPtr = GetSamplerPtr(sampler); if (!samPtr) return;

    auto& pipeline = m_computePipelines[m_currentComputePipeline - 1];
    const auto& layouts = pipeline->GetDescriptorSetLayouts();
    if (layouts.empty()) return;

    auto it = m_computeDescriptorSets.find(m_currentComputePipeline);
    std::shared_ptr<G::IDescriptorSet> descSet;
    if (it != m_computeDescriptorSets.end()) {
        descSet = it->second;
    } else {
        descSet = fac->CreateDescriptorSet(layouts[0].get());
        if (!descSet) return;
        m_computeDescriptorSets[m_currentComputePipeline] = descSet;
    }

    descSet->BindTexture(slot, texPtr.get(), samPtr.get());
    descSet->Update();
    m_cmdBuffer->BindDescriptorSet(0, descSet.get());
}

void SRPGraphicsAPI::CmdBindStorageImage(uint32_t slot, TextureHandle tex) {
    if (!m_cmdBuffer || m_currentComputePipeline == 0) return;
    auto* fac = GetFac(); if (!fac) return;
    auto texPtr = GetTexturePtr(tex); if (!texPtr) return;

    auto& pipeline = m_computePipelines[m_currentComputePipeline - 1];
    const auto& layouts = pipeline->GetDescriptorSetLayouts();
    if (layouts.empty()) return;

    auto it = m_computeDescriptorSets.find(m_currentComputePipeline);
    std::shared_ptr<G::IDescriptorSet> descSet;
    if (it != m_computeDescriptorSets.end()) {
        descSet = it->second;
    } else {
        descSet = fac->CreateDescriptorSet(layouts[0].get());
        if (!descSet) return;
        m_computeDescriptorSets[m_currentComputePipeline] = descSet;
    }

    descSet->BindStorageImage(slot, texPtr.get());
    descSet->Update();
    m_cmdBuffer->BindDescriptorSet(0, descSet.get());
}

void SRPGraphicsAPI::CmdBindStorageBuffer(uint32_t slot, BufferHandle buf) {
    if (!m_cmdBuffer || m_currentComputePipeline == 0) return;
    auto* fac = GetFac(); if (!fac) return;

    auto bufPtr = (buf > 0 && buf - 1 < m_buffers.size()) ? m_buffers[buf - 1].get() : nullptr;
    if (!bufPtr) return;

    auto& pipeline = m_computePipelines[m_currentComputePipeline - 1];
    const auto& layouts = pipeline->GetDescriptorSetLayouts();
    if (layouts.empty()) return;

    auto it = m_computeDescriptorSets.find(m_currentComputePipeline);
    std::shared_ptr<G::IDescriptorSet> descSet;
    if (it != m_computeDescriptorSets.end()) {
        descSet = it->second;
    } else {
        descSet = fac->CreateDescriptorSet(layouts[0].get());
        if (!descSet) return;
        m_computeDescriptorSets[m_currentComputePipeline] = descSet;
    }

    descSet->BindBuffer(slot, bufPtr, 0, 0, G::DescriptorType::StorageBuffer);
    descSet->Update();
    m_cmdBuffer->BindDescriptorSet(0, descSet.get());
}

void SRPGraphicsAPI::Shutdown() {
    m_shaders.clear(); m_pipelines.clear(); m_renderTargets.clear();
    m_depthTargets.clear(); m_buffers.clear(); m_textures.clear();
    m_samplers.clear(); m_frameDescriptorSets.clear();
    m_computePipelines.clear(); m_computeDescriptorSets.clear();
    m_cmdBuffer = nullptr; m_currentPipeline = 0; m_currentComputePipeline = 0;
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
uint32_t SRP_CreateSampler(const SRPSamplerDesc* d) { return d ? SRPGraphicsAPI::Get().CreateSampler(*d) : 0; }
void SRP_DestroySampler(uint32_t h) { SRPGraphicsAPI::Get().DestroySampler(h); }
void SRP_CmdBindTexture(uint32_t slot, uint32_t tex, uint32_t sampler) { SRPGraphicsAPI::Get().CmdBindTexture(slot, tex, sampler); }
void SRP_CmdBlitRenderTarget(uint32_t dst) { SRPGraphicsAPI::Get().CmdBlitRenderTarget(dst); }

// Compute pipeline C wrappers
uint32_t SRP_CreateComputePipeline(uint32_t shader, uint32_t pcs) { return SRPGraphicsAPI::Get().CreateComputePipeline(shader, pcs); }
void SRP_DestroyComputePipeline(uint32_t h) { SRPGraphicsAPI::Get().DestroyComputePipeline(h); }
void SRP_CmdBindComputePipeline(uint32_t h) { SRPGraphicsAPI::Get().CmdBindComputePipeline(h); }
void SRP_CmdDispatch(uint32_t x, uint32_t y, uint32_t z) { SRPGraphicsAPI::Get().CmdDispatch(x, y, z); }
void SRP_CmdBindComputeTexture(uint32_t slot, uint32_t tex, uint32_t sampler) { SRPGraphicsAPI::Get().CmdBindComputeTexture(slot, tex, sampler); }
void SRP_CmdBindStorageImage(uint32_t slot, uint32_t tex) { SRPGraphicsAPI::Get().CmdBindStorageImage(slot, tex); }
void SRP_CmdBindStorageBuffer(uint32_t slot, uint32_t buf) { SRPGraphicsAPI::Get().CmdBindStorageBuffer(slot, buf); }

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
