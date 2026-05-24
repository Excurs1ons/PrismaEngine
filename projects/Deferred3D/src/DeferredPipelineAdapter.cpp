#include "DeferredPipelineAdapter.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/ISwapChain.h"
#include "graphic/MeshRenderer.h"
#include "graphic/Mesh.h"
#include "graphic/Renderer.h"
#include "graphic/adapters/vulkan/VulkanResources.h"
#include "graphic/adapters/vulkan/VulkanPipelineState.h"
#include "app/Engine.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "Logger.h"
#include <algorithm>

namespace Prisma {

static std::shared_ptr<Graphic::ITexture> CreateRT(Graphic::IRenderDevice* dev, uint32_t w, uint32_t h, Graphic::TextureFormat fmt, bool isDepth = false)
{
    Graphic::TextureDesc d;
    d.width = w; d.height = h; d.format = fmt;
    if (isDepth) { d.allowDepthStencil = true; d.allowShaderResource = true; }
    else { d.allowRenderTarget = true; d.allowShaderResource = true; }
    return std::shared_ptr<Graphic::ITexture>(dev->GetResourceFactory()->CreateTextureImpl(d).release());
}

static VkImageView GetVkImageView(const std::shared_ptr<Graphic::ITexture>& tex)
{
    auto* vk = dynamic_cast<Graphic::Vulkan::VulkanTexture*>(tex.get());
    return vk ? vk->GetVkImageView() : VK_NULL_HANDLE;
}

static std::shared_ptr<Graphic::IPipelineState> MakePSO(
    Graphic::IRenderDevice* dev,
    const char* vert, const char* frag,
    VkRenderPass rp,
    const std::vector<Graphic::TextureFormat>& rtFormats,
    Graphic::TextureFormat depthFmt = Graphic::TextureFormat::Unknown,
    bool depthTest = false, bool cull = false)
{
    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!rm) return nullptr;
    auto vs = rm->LoadShaderSync(vert, "main");
    auto fs = rm->LoadShaderSync(frag, "main");
    if (!vs || !fs) { LOG_ERROR("MakePSO", "shader fail {} {}", vert, frag); return nullptr; }

    auto pso = dev->GetResourceFactory()->CreatePipelineStateImpl();
    pso->SetShader(Graphic::ShaderType::Vertex, vs);
    pso->SetShader(Graphic::ShaderType::Pixel, fs);
    pso->SetPrimitiveTopology(Graphic::PrimitiveTopology::TriangleList);

    Graphic::RasterizerState rs;
    rs.cullMode = cull ? Graphic::CullMode::Back : Graphic::CullMode::None;
    pso->SetRasterizerState(rs);

    Graphic::DepthStencilState ds;
    ds.depthEnable = depthTest;
    ds.depthWriteEnable = depthTest;
    if (depthTest) ds.depthFunc = Graphic::ComparisonFunc::Less;
    pso->SetDepthStencilState(ds);

    Graphic::BlendState bs;
    bs.blendEnable = false;
    pso->SetBlendState(bs);

    pso->SetRenderTargetFormats(rtFormats);
    if (depthFmt != Graphic::TextureFormat::Unknown) pso->SetDepthStencilFormat(depthFmt);

    auto* vkPSO = dynamic_cast<Graphic::Vulkan::VulkanPipelineState*>(pso.get());
    if (vkPSO && rp) vkPSO->SetCustomRenderPass(rp);

    if (!pso->Create(dev)) {
        LOG_ERROR("Deferred3D_PSO", "FAILED: vert={} frag={} errors=\"{}\"", vert, frag, pso->GetErrors());
        return nullptr;
    }
    LOG_INFO("Deferred3D_PSO", "OK: vert={} frag={}", vert, frag);
    return std::shared_ptr<Graphic::IPipelineState>(std::move(pso));
}

void UploadMeshGPUBuffers(Graphic::IRenderDevice* device, Scene* scene)
{
    if (!device || !scene) return;
    auto* factory = device->GetResourceFactory();
    if (!factory) return;
    int mc = 0, tv = 0, ti = 0;
    for (const auto& node : scene->GetNodes()) {
        auto mr = scene->GetComponent<Graphic::MeshRenderer>(node);
        if (!mr) continue;
        auto mesh = mr->GetMesh();
        if (!mesh || !mesh->HasCPUMeshData()) continue;
        auto& subs = const_cast<std::vector<Graphic::SubMeshBuffer>&>(mesh->GetSubMeshes());
        const auto& cpus = mesh->GetCPUSubMeshes();
        for (size_t i = 0; i < subs.size() && i < cpus.size(); ++i) {
            auto& sub = subs[i];
            if (sub.vertexBuffer && sub.indexBuffer) continue;
            const auto& cpu = cpus[i];
            std::vector<Graphic::Vertex> verts(cpu.positions.size());
            for (size_t v = 0; v < cpu.positions.size(); ++v) {
                verts[v].position = Graphic::PrismaMath::vec4(cpu.positions[v].x, cpu.positions[v].y, cpu.positions[v].z, 1);
                if (v < cpu.normals.size()) verts[v].normal = Graphic::PrismaMath::vec4(cpu.normals[v].x, cpu.normals[v].y, cpu.normals[v].z, 0);
                if (v < cpu.uvs.size()) verts[v].texCoord = Graphic::PrismaMath::vec4(cpu.uvs[v].x, cpu.uvs[v].y, 0, 0);
                verts[v].color = Graphic::PrismaMath::vec4(1,1,1,1);
            }
            Graphic::BufferDesc vb;
            vb.type = Graphic::BufferType::Vertex;
            vb.size = verts.size() * sizeof(Graphic::Vertex);
            vb.initialData = verts.data();
            vb.usage = Graphic::BufferUsage::Immutable;
            sub.vertexBuffer.reset(factory->CreateBufferImpl(vb).release());
            sub.vertexCount = (uint32_t)verts.size();
            Graphic::BufferDesc ib;
            ib.type = Graphic::BufferType::Index;
            ib.size = cpu.indices.size() * sizeof(uint32_t);
            ib.initialData = cpu.indices.data();
            ib.usage = Graphic::BufferUsage::Immutable;
            sub.indexBuffer.reset(factory->CreateBufferImpl(ib).release());
            sub.indexCount = (uint32_t)cpu.indices.size();
            mc++; tv += (int)verts.size(); ti += (int)cpu.indices.size();
        }
    }
    LOG_INFO("Deferred3D_Init", "GPU upload: {} meshes {} verts {} idx", mc, tv, ti);
}

// ========================================================================
DeferredPipelineAdapter::DeferredPipelineAdapter() : m_device(nullptr), m_vkDevice(VK_NULL_HANDLE) {}
DeferredPipelineAdapter::~DeferredPipelineAdapter() { Shutdown(); }

int DeferredPipelineAdapter::Initialize(Graphic::IRenderDevice* device)
{
    m_device = device;
    if (!m_device) return -1;
    m_vkDevice = m_device->GetVkDevice();

    m_pipeline = std::make_shared<Graphic::DeferredPipeline>();
    m_pipeline->Initialize();

    auto* factory = m_device->GetResourceFactory();
    if (factory) {
        Graphic::SamplerDesc sd;
        sd.filter = Graphic::TextureFilter::Linear;
        sd.addressU = Graphic::TextureAddressMode::Clamp;
        sd.addressV = Graphic::TextureAddressMode::Clamp;
        sd.addressW = Graphic::TextureAddressMode::Clamp;
        auto samplerPtr = factory->CreateSamplerImpl(sd);
        if (!samplerPtr) LOG_ERROR("Deferred3D_Init", "CreateSamplerImpl returned null!");
        m_defaultSampler = std::shared_ptr<Graphic::ISampler>(samplerPtr.release());
        LOG_INFO("Deferred3D_Init", "Sampler created: ptr={}", !!m_defaultSampler);
    } else {
        LOG_ERROR("Deferred3D_Init", "factory is null!");
    }

    if (!CreateResources(1920, 1080)) return -1;

    auto* sm = Engine::Get().GetSceneManager();
    if (sm) UploadMeshGPUBuffers(m_device, sm->GetCurrentScene());

    LOG_INFO("Deferred3D_Init", "adapter initialized");
    return 0;
}

void DeferredPipelineAdapter::Shutdown()
{
    DestroyResources();
    m_pipeline.reset();
    m_gbufferPSO.reset();
    m_lightingPSO.reset();
    m_ssgiPSO.reset();
    m_compositePSO.reset();
    m_forwardPSO.reset();
    m_debugGBufferPSO.reset();
    m_lightingDS.reset();
    m_ssgiDS.reset();
    m_compositeDS.reset();
    for (auto& ds : m_debugGBufferDS) ds.reset();
}

bool DeferredPipelineAdapter::CreateResources(uint32_t w, uint32_t h)
{
    DestroyResources();
    m_width = w; m_height = h;

    m_gbPosition = CreateRT(m_device, w, h, Graphic::TextureFormat::RGBA16_Float);
    m_gbNormal   = CreateRT(m_device, w, h, Graphic::TextureFormat::RGBA16_Float);
    m_gbAlbedo   = CreateRT(m_device, w, h, Graphic::TextureFormat::RGBA8_UNorm);
    m_gbEmissive = CreateRT(m_device, w, h, Graphic::TextureFormat::RGBA16_Float);
    m_gbDepth    = CreateRT(m_device, w, h, Graphic::TextureFormat::D32_Float, true);
    m_lightingOutput = CreateRT(m_device, w, h, Graphic::TextureFormat::RGBA16_Float);
    m_ssgiOutput     = CreateRT(m_device, w, h, Graphic::TextureFormat::RGBA16_Float);

    if (!m_gbPosition || !m_gbNormal || !m_gbAlbedo || !m_gbEmissive || !m_gbDepth || !m_lightingOutput || !m_ssgiOutput)
        return false;

    VkImageView gbViews[4] = { GetVkImageView(m_gbPosition), GetVkImageView(m_gbNormal), GetVkImageView(m_gbAlbedo), GetVkImageView(m_gbEmissive) };
    VkImageView depthView = GetVkImageView(m_gbDepth);
    VkImageView lightView = GetVkImageView(m_lightingOutput);

    // --- GBuffer RP (4 color + depth) ---
    VkAttachmentDescription gbColor[4] = {};
    VkAttachmentReference gbColorRef[4] = {};
    for (int i = 0; i < 4; ++i) {
        gbColor[i].format = (i == 2) ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R16G16B16A16_SFLOAT;
        gbColor[i].samples = VK_SAMPLE_COUNT_1_BIT;
        gbColor[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        gbColor[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        gbColor[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        gbColor[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        gbColorRef[i].attachment = (uint32_t)i;
        gbColorRef[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentDescription gbDepth = {};
    gbDepth.format = VK_FORMAT_D32_SFLOAT;
    gbDepth.samples = VK_SAMPLE_COUNT_1_BIT;
    gbDepth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    gbDepth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    gbDepth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    gbDepth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference gbDepthRef = { 4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkAttachmentDescription gbAllAttachments[5] = { gbColor[0], gbColor[1], gbColor[2], gbColor[3], gbDepth };
    VkSubpassDescription gbSubpass = {};
    gbSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    gbSubpass.colorAttachmentCount = 4;
    gbSubpass.pColorAttachments = gbColorRef;
    gbSubpass.pDepthStencilAttachment = &gbDepthRef;

    VkSubpassDependency gbDeps[2] = {};
    gbDeps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    gbDeps[0].dstSubpass = 0;
    gbDeps[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    gbDeps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    gbDeps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    gbDeps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    gbDeps[1].srcSubpass = 0;
    gbDeps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    gbDeps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    gbDeps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    gbDeps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    gbDeps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkRenderPassCreateInfo rpInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rpInfo.attachmentCount = 5;
    rpInfo.pAttachments = gbAllAttachments;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &gbSubpass;
    rpInfo.dependencyCount = 2;
    rpInfo.pDependencies = gbDeps;
    if (vkCreateRenderPass(m_vkDevice, &rpInfo, nullptr, &m_gbufferRP) != VK_SUCCESS) return false;

    VkImageView gbFBViews[5] = { gbViews[0], gbViews[1], gbViews[2], gbViews[3], depthView };
    VkFramebufferCreateInfo fbInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    fbInfo.renderPass = m_gbufferRP;
    fbInfo.attachmentCount = 5;
    fbInfo.pAttachments = gbFBViews;
    fbInfo.width = w; fbInfo.height = h; fbInfo.layers = 1;
    if (vkCreateFramebuffer(m_vkDevice, &fbInfo, nullptr, &m_gbufferFB) != VK_SUCCESS) return false;

    // --- Lighting RP (1 color) ---
    VkAttachmentDescription ltColor = {};
    ltColor.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    ltColor.samples = VK_SAMPLE_COUNT_1_BIT;
    ltColor.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ltColor.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ltColor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ltColor.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference ltColorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription ltSubpass = {};
    ltSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    ltSubpass.colorAttachmentCount = 1;
    ltSubpass.pColorAttachments = &ltColorRef;

    VkSubpassDependency ltDep[2] = {};
    ltDep[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    ltDep[0].dstSubpass = 0;
    ltDep[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    ltDep[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    ltDep[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    ltDep[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    ltDep[1].srcSubpass = 0;
    ltDep[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    ltDep[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    ltDep[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    ltDep[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    ltDep[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkRenderPassCreateInfo ltRP = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    ltRP.attachmentCount = 1;
    ltRP.pAttachments = &ltColor;
    ltRP.subpassCount = 1;
    ltRP.pSubpasses = &ltSubpass;
    ltRP.dependencyCount = 2;
    ltRP.pDependencies = ltDep;
    if (vkCreateRenderPass(m_vkDevice, &ltRP, nullptr, &m_lightingRP) != VK_SUCCESS) return false;

    VkFramebufferCreateInfo ltFB = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    ltFB.renderPass = m_lightingRP;
    ltFB.attachmentCount = 1;
    ltFB.pAttachments = &lightView;
    ltFB.width = w; ltFB.height = h; ltFB.layers = 1;
    if (vkCreateFramebuffer(m_vkDevice, &ltFB, nullptr, &m_lightingFB) != VK_SUCCESS) return false;

    // --- SSGI RP ---
    VkImageView ssgiView = GetVkImageView(m_ssgiOutput);
    VkAttachmentDescription ssgiColor = ltColor; // same format and layout as lighting
    VkAttachmentReference ssgiColorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription ssgiSubpass = {};
    ssgiSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    ssgiSubpass.colorAttachmentCount = 1;
    ssgiSubpass.pColorAttachments = &ssgiColorRef;

    VkSubpassDependency ssgiDep[2] = {};
    ssgiDep[0].srcSubpass = VK_SUBPASS_EXTERNAL; ssgiDep[0].dstSubpass = 0;
    ssgiDep[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    ssgiDep[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    ssgiDep[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    ssgiDep[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    ssgiDep[1].srcSubpass = 0; ssgiDep[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    ssgiDep[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    ssgiDep[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    ssgiDep[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    ssgiDep[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkRenderPassCreateInfo ssgiRP = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    ssgiRP.attachmentCount = 1;
    ssgiRP.pAttachments = &ssgiColor;
    ssgiRP.subpassCount = 1;
    ssgiRP.pSubpasses = &ssgiSubpass;
    ssgiRP.dependencyCount = 2;
    ssgiRP.pDependencies = ssgiDep;
    if (vkCreateRenderPass(m_vkDevice, &ssgiRP, nullptr, &m_ssgiRP) != VK_SUCCESS) return false;

    VkFramebufferCreateInfo ssgiFB = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    ssgiFB.renderPass = m_ssgiRP;
    ssgiFB.attachmentCount = 1;
    ssgiFB.pAttachments = &ssgiView;
    ssgiFB.width = w; ssgiFB.height = h; ssgiFB.layers = 1;
    if (vkCreateFramebuffer(m_vkDevice, &ssgiFB, nullptr, &m_ssgiFB) != VK_SUCCESS) return false;

    CreatePSOs();
    return true;
}

void DeferredPipelineAdapter::DestroyResources()
{
    if (m_gbufferFB) { vkDestroyFramebuffer(m_vkDevice, m_gbufferFB, nullptr); m_gbufferFB = VK_NULL_HANDLE; }
    if (m_gbufferRP) { vkDestroyRenderPass(m_vkDevice, m_gbufferRP, nullptr); m_gbufferRP = VK_NULL_HANDLE; }
    if (m_lightingFB) { vkDestroyFramebuffer(m_vkDevice, m_lightingFB, nullptr); m_lightingFB = VK_NULL_HANDLE; }
    if (m_lightingRP) { vkDestroyRenderPass(m_vkDevice, m_lightingRP, nullptr); m_lightingRP = VK_NULL_HANDLE; }
    if (m_ssgiFB) { vkDestroyFramebuffer(m_vkDevice, m_ssgiFB, nullptr); m_ssgiFB = VK_NULL_HANDLE; }
    if (m_ssgiRP) { vkDestroyRenderPass(m_vkDevice, m_ssgiRP, nullptr); m_ssgiRP = VK_NULL_HANDLE; }
    m_gbPosition.reset(); m_gbNormal.reset(); m_gbAlbedo.reset(); m_gbEmissive.reset(); m_gbDepth.reset();
    m_lightingOutput.reset(); m_ssgiOutput.reset();
}

void DeferredPipelineAdapter::CreatePSOs()
{
    m_gbufferPSO = MakePSO(m_device,
        "assets/shaders/gbuffer.vert.spv", "assets/shaders/gbuffer.frag.spv",
        m_gbufferRP,
        {Graphic::TextureFormat::RGBA16_Float, Graphic::TextureFormat::RGBA16_Float,
         Graphic::TextureFormat::RGBA8_UNorm, Graphic::TextureFormat::RGBA16_Float},
        Graphic::TextureFormat::D32_Float, true, false);

    auto* rm = Engine::Get().GetRenderResourceManager();

    // SSGI PSO: fullscreen triangle, samples GBuffer → writes indirect lighting
    m_ssgiPSO = MakePSO(m_device,
        "assets/shaders/deferred_fullscreen.vert.spv", "assets/shaders/ssgi.frag.spv",
        m_ssgiRP,
        {Graphic::TextureFormat::RGBA16_Float}, Graphic::TextureFormat::Unknown, false, false);

    m_lightingPSO = MakePSO(m_device,
        "assets/shaders/deferred_fullscreen.vert.spv", "assets/shaders/deferred_lighting.frag.spv",
        m_lightingRP,
        {Graphic::TextureFormat::RGBA16_Float}, Graphic::TextureFormat::Unknown, false, false);

    m_compositePSO = MakePSO(m_device,
        "assets/shaders/deferred_fullscreen.vert.spv", "assets/shaders/deferred_composite.frag.spv",
        VK_NULL_HANDLE,
        {Graphic::TextureFormat::RGBA8_UNorm}, Graphic::TextureFormat::Unknown, false, false);

    // Lighting descriptor set: reads 4 GBuffer textures
    if (m_lightingPSO) {
        auto layouts = m_lightingPSO->GetDescriptorSetLayouts();
        LOG_INFO("Deferred3D_PSO", "Lighting PSO DS layouts: {}", layouts.size());
        if (!layouts.empty()) {
            LOG_INFO("Deferred3D_PSO", "Lighting DS layout[0] native={}",
                     layouts[0] ? layouts[0]->GetNativeHandle() : 0);
            m_lightingDS = m_device->GetResourceFactory()->CreateDescriptorSet(layouts[0].get());
            if (!m_lightingDS) LOG_ERROR("Deferred3D_PSO", "CreateDescriptorSet returned null!");
            if (m_lightingDS && m_defaultSampler) {
                m_lightingDS->BindTexture(0, m_gbPosition.get(), m_defaultSampler.get());
                m_lightingDS->BindTexture(1, m_gbNormal.get(), m_defaultSampler.get());
                m_lightingDS->BindTexture(2, m_gbAlbedo.get(), m_defaultSampler.get());
                m_lightingDS->BindTexture(3, m_gbEmissive.get(), m_defaultSampler.get());
                m_lightingDS->Update();
                LOG_INFO("Deferred3D_PSO", "Lighting DS created and bound");
            } else {
                LOG_ERROR("Deferred3D_PSO", "Lighting DS: DS={} Sampler={}", !!m_lightingDS, !!m_defaultSampler);
            }
        }
    } else {
        LOG_ERROR("Deferred3D_PSO", "m_lightingPSO is null!");
    }

    // SSGI descriptor set: reads 5 GBuffer textures
    if (m_ssgiPSO) {
        auto layouts = m_ssgiPSO->GetDescriptorSetLayouts();
        LOG_INFO("Deferred3D_PSO", "SSGI PSO DS layouts: {}", layouts.size());
        if (!layouts.empty()) {
            m_ssgiDS = m_device->GetResourceFactory()->CreateDescriptorSet(layouts[0].get());
            if (m_ssgiDS && m_defaultSampler) {
                m_ssgiDS->BindTexture(0, m_gbPosition.get(), m_defaultSampler.get());
                m_ssgiDS->BindTexture(1, m_gbNormal.get(), m_defaultSampler.get());
                m_ssgiDS->BindTexture(2, m_gbAlbedo.get(), m_defaultSampler.get());
                m_ssgiDS->BindTexture(3, m_gbEmissive.get(), m_defaultSampler.get());
                m_ssgiDS->BindTexture(4, m_gbDepth.get(), m_defaultSampler.get());
                m_ssgiDS->BindTexture(5, m_lightingOutput.get(), m_defaultSampler.get());
                m_ssgiDS->Update();
                LOG_INFO("Deferred3D_PSO", "SSGI DS created (5 GBuffer + lightingOutput)");
            }
        }
    }

    // Forward PSO: renders to swapchain with material colors
    m_forwardPSO = MakePSO(m_device,
        "assets/shaders/forward.vert.spv", "assets/shaders/forward.frag.spv",
        VK_NULL_HANDLE,  // swapchain RP
        {Graphic::TextureFormat::RGBA8_UNorm},
        Graphic::TextureFormat::D32_Float, true, false);

    // Composite descriptor set: reads both lighting output and SSGI
    if (m_compositePSO) {
        auto layouts = m_compositePSO->GetDescriptorSetLayouts();
        LOG_INFO("Deferred3D_PSO", "Composite PSO DS layouts: {}", layouts.size());
        if (!layouts.empty()) {
            m_compositeDS = m_device->GetResourceFactory()->CreateDescriptorSet(layouts[0].get());
            if (m_compositeDS && m_defaultSampler) {
                m_compositeDS->BindTexture(0, m_lightingOutput.get(), m_defaultSampler.get());
                m_compositeDS->BindTexture(1, m_ssgiOutput.get(), m_defaultSampler.get());
                m_compositeDS->Update();
                LOG_INFO("Deferred3D_PSO", "Composite DS bound to lightingOutput + ssgiOutput");
            }
        }
    }

    // Debug GBuffer PSO: fullscreen triangle sampling one GBuffer RT
    m_debugGBufferPSO = MakePSO(m_device,
        "assets/shaders/deferred_fullscreen.vert.spv", "assets/shaders/debug_gbuffer.frag.spv",
        VK_NULL_HANDLE,
        {Graphic::TextureFormat::RGBA8_UNorm}, Graphic::TextureFormat::Unknown, false, false);
    if (m_debugGBufferPSO) {
        auto layouts = m_debugGBufferPSO->GetDescriptorSetLayouts();
        if (!layouts.empty()) {
            std::shared_ptr<Graphic::ITexture> gbTexs[5] = {m_gbPosition, m_gbNormal, m_gbAlbedo, m_gbEmissive, m_gbDepth};
            const char* gbNames[5] = {"gbPosition", "gbNormal", "gbAlbedo", "gbEmissive", "gbDepth"};
            for (int i = 0; i < 5; ++i) {
                m_debugGBufferDS[i] = m_device->GetResourceFactory()->CreateDescriptorSet(layouts[0].get());
                if (m_debugGBufferDS[i] && m_defaultSampler && gbTexs[i]) {
                    m_debugGBufferDS[i]->BindTexture(0, gbTexs[i].get(), m_defaultSampler.get());
                    m_debugGBufferDS[i]->Update();
                    LOG_INFO("Deferred3D_PSO", "Debug GBuffer DS[{}] = {}", i, gbNames[i]);
                }
            }
        }
    }
}

void DeferredPipelineAdapter::OnSceneLoaded(Scene* scene) {}

void DeferredPipelineAdapter::Execute(const Graphic::RenderContext& ctx)
{
    if (!m_device || !ctx.commandBuffer) return;
    auto* vkCmd = static_cast<VkCommandBuffer>(ctx.commandBuffer->GetNativeHandle());
    if (!vkCmd) return;

    // Handle resize
    if (ctx.width != m_width || ctx.height != m_height) {
        CreateResources(ctx.width, ctx.height);
        CreatePSOs();
    }

    m_cameraAdapter.SetCameraData(ctx.camera, (float)ctx.width, (float)ctx.height);
    if (m_pipeline) m_pipeline->Update(Prisma::Timestep(ctx.deltaTime), &m_cameraAdapter);

    // ====================================================================
    // Phase 1: GBuffer Pass — render scene geometry to 4 RTs + depth
    // ====================================================================
    {
        VkClearValue gbClear[5];
        for (int i = 0; i < 4; ++i) gbClear[i].color = {{0,0,0,0}};
        gbClear[4].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo rp = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rp.renderPass = m_gbufferRP;
        rp.framebuffer = m_gbufferFB;
        rp.renderArea = {{0,0}, {m_width, m_height}};
        rp.clearValueCount = 5;
        rp.pClearValues = gbClear;
        vkCmdBeginRenderPass(vkCmd, &rp, VK_SUBPASS_CONTENTS_INLINE);
    }

    ctx.commandBuffer->SetViewport({0,0,(float)m_width,(float)m_height,0,1});
    ctx.commandBuffer->SetScissorRect({0,0,(int)m_width,(int)m_height});
    ctx.commandBuffer->SetPipelineState(m_gbufferPSO.get());

    const auto& cmds = Graphic::Renderer::GetCommandQueue();
    if (!m_firstFrameLogged) {
        LOG_INFO("Deferred3D_GBuffer", "GBuffer pass: {} render commands, queue ptr={}", cmds.size(), (void*)&cmds);
        for (size_t i = 0; i < cmds.size() && i < 5; ++i) {
            const auto& c = cmds[i];
            LOG_INFO("Deferred3D_GBuffer", "  cmd[{}]: mesh={} mat={} vb={} ib={}",
                     i, (void*)c.mesh, (void*)c.material,
                     c.mesh && !c.mesh->GetSubMeshes().empty() ? (void*)c.mesh->GetSubMeshes()[0].vertexBuffer.get() : nullptr,
                     c.mesh && !c.mesh->GetSubMeshes().empty() ? (void*)c.mesh->GetSubMeshes()[0].indexBuffer.get() : nullptr);
        }
        // 首帧打印所有材质的 BaseColor，用于排查颜色问题
        for (size_t i = 0; i < cmds.size(); ++i) {
            const auto& c = cmds[i];
            if (!c.material) {
                LOG_INFO("Deferred3D_Material", "  cmd[{}]: NO material (nullptr)", i);
                continue;
            }
            std::string matName = c.material->GetName();
            LOG_INFO("Deferred3D_Material", "  cmd[{}]: mat='{}' ptr={}", i, matName, (void*)c.material);

            // 打印所有支持的参数
            auto* bc = c.material->GetParam("BaseColor");
            auto* bcl = c.material->GetParam("basecolor");
            if (bc) {
                if (const auto* v4 = std::get_if<Graphic::PrismaMath::vec4>(bc))
                    LOG_INFO("Deferred3D_Material", "    BaseColor (uppercase) = ({:.3},{:.3},{:.3},{:.3})",
                             v4->r, v4->g, v4->b, v4->a);
            } else {
                LOG_INFO("Deferred3D_Material", "    BaseColor (uppercase) = NOT FOUND");
            }
            if (bcl) {
                if (const auto* v4 = std::get_if<Graphic::PrismaMath::vec4>(bcl))
                    LOG_INFO("Deferred3D_Material", "    basecolor (lowercase) = ({:.3},{:.3},{:.3},{:.3})",
                             v4->r, v4->g, v4->b, v4->a);
            } else {
                LOG_INFO("Deferred3D_Material", "    basecolor (lowercase) = NOT FOUND");
            }
        }
    }
    for (const auto& c : cmds) {
        if (!c.mesh) continue;

        Graphic::PrismaMath::mat4 vp = ctx.camera.projectionMatrix * ctx.camera.viewMatrix;

        Graphic::PrismaMath::vec4 baseCol(0.8f, 0.8f, 0.8f, 1);
        if (c.material) {
            const auto* p = c.material->GetParam("BaseColor");
            if (!p) p = c.material->GetParam("basecolor");
            if (p) {
                if (const auto* col = std::get_if<Prisma::Color>(p))
                    baseCol = {col->r, col->g, col->b, col->a};
                else if (const auto* v4 = std::get_if<Graphic::PrismaMath::vec4>(p))
                    baseCol = *v4;
            }
        }

        Graphic::PrismaMath::vec4 emissive(0.0f, 0.0f, 0.0f, 0.0f);
        if (c.material) {
            const auto* pe = c.material->GetParam("Emissive");
            if (!pe) pe = c.material->GetParam("emissive");
            if (pe) {
                if (const auto* v4 = std::get_if<Graphic::PrismaMath::vec4>(pe))
                    emissive = *v4;
            }
        }

        struct PushData { Graphic::PrismaMath::mat4 vp; Graphic::PrismaMath::mat4 model; Graphic::PrismaMath::vec4 color; Graphic::PrismaMath::vec4 emissive; };
        PushData pd{};
        pd.vp = vp;
        pd.model = c.transform;
        pd.color = baseCol;
        pd.emissive = emissive;
        ctx.commandBuffer->PushConstants(Graphic::ShaderType::Unknown, &pd, sizeof(pd));

        for (const auto& sm : c.mesh->GetSubMeshes()) {
            if (sm.vertexBuffer && sm.indexBuffer) {
                ctx.commandBuffer->SetVertexBuffer(sm.vertexBuffer.get(), 0);
                ctx.commandBuffer->SetIndexBuffer(sm.indexBuffer.get());
                ctx.commandBuffer->DrawIndexed(sm.indexCount);
            }
        }
    }

    vkCmdEndRenderPass(vkCmd);

    // ====================================================================
    // Phase 1.5: Lighting Pass — reads GBuffer → lightingOutput (先光照，给 SSGI 反弹用)
    // ====================================================================
    {
        VkClearValue ltClear;
        ltClear.color = {{0, 0, 0, 0}};
        VkRenderPassBeginInfo rp = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rp.renderPass = m_lightingRP;
        rp.framebuffer = m_lightingFB;
        rp.renderArea = {{0, 0}, {m_width, m_height}};
        rp.clearValueCount = 1;
        rp.pClearValues = &ltClear;
        vkCmdBeginRenderPass(vkCmd, &rp, VK_SUBPASS_CONTENTS_INLINE);
    }
    ctx.commandBuffer->SetViewport({0,0,(float)m_width,(float)m_height,0,1});
    ctx.commandBuffer->SetScissorRect({0,0,(int)m_width,(int)m_height});
    ctx.commandBuffer->SetPipelineState(m_lightingPSO.get());
    ctx.commandBuffer->BindDescriptorSet(0, m_lightingDS.get());

    {
        struct LightingPC { Graphic::PrismaMath::vec4 ambient; Graphic::PrismaMath::vec4 lightDir; Graphic::PrismaMath::vec4 lightColor; };
        LightingPC lpc{};
        // glm::vec4 默认构造不初始化，必须显式清零
        lpc.ambient = Graphic::PrismaMath::vec4(0.0f);
        lpc.lightDir = Graphic::PrismaMath::vec4(0.0f);
        lpc.lightColor = Graphic::PrismaMath::vec4(0.0f);

        // 从场景光源中提取环境光（Ambient）和定向光（Directional）
        for (const auto& light : ctx.lights) {
            int lightType = static_cast<int>(light.direction.w + 0.5f);
            PrismaMath::vec3 col = PrismaMath::vec3(light.color.x, light.color.y, light.color.z);
            float intensity = glm::length(col);
            if (intensity < 0.001f) continue;

            if (lightType == static_cast<int>(Graphic::DeferredPipeline::LightType::Directional)) { // Directional
                PrismaMath::vec3 dir = PrismaMath::vec3(light.direction.x, light.direction.y, light.direction.z);
                float len = glm::length(dir);
                if (len > 0.001f) {
                    dir = -dir / len;
                    lpc.lightDir = {dir.x, dir.y, dir.z, 0.0f};
                    lpc.lightColor = {col.x, col.y, col.z, 1.0f};
                }
                if (!m_firstFrameLogged)
                    LOG_INFO("Deferred3D_Light", "Directional: dir=({:.3},{:.3},{:.3}) color=({:.3},{:.3},{:.3})",
                             lpc.lightDir.x, lpc.lightDir.y, lpc.lightDir.z, col.x, col.y, col.z);
            } else if (lightType == static_cast<int>(Graphic::DeferredPipeline::LightType::Ambient)) { // Ambient
                lpc.ambient = {col.x, col.y, col.z, 1.0f};
                if (!m_firstFrameLogged)
                    LOG_INFO("Deferred3D_Light", "Ambient: color=({:.3},{:.3},{:.3})", col.x, col.y, col.z);
            }
        }

        ctx.commandBuffer->PushConstants(Graphic::ShaderType::Unknown, &lpc, sizeof(lpc));
        ctx.commandBuffer->Draw(3, 1);
    }
    vkCmdEndRenderPass(vkCmd);

    // ====================================================================
    // Phase 2: SSGI Pass — reads GBuffer + lightingOutput → ssgiOutput
    // ====================================================================
    if (m_ssgiPSO && m_ssgiDS) {
        {
            VkClearValue ssgiClear;
            ssgiClear.color = {{0, 0, 0, 0}};
            VkRenderPassBeginInfo rp = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
            rp.renderPass = m_ssgiRP;
            rp.framebuffer = m_ssgiFB;
            rp.renderArea = {{0, 0}, {m_width, m_height}};
            rp.clearValueCount = 1;
            rp.pClearValues = &ssgiClear;
            vkCmdBeginRenderPass(vkCmd, &rp, VK_SUBPASS_CONTENTS_INLINE);
        }
        ctx.commandBuffer->SetViewport({0,0,(float)m_width,(float)m_height,0,1});
        ctx.commandBuffer->SetScissorRect({0,0,(int)m_width,(int)m_height});
        ctx.commandBuffer->SetPipelineState(m_ssgiPSO.get());
        ctx.commandBuffer->BindDescriptorSet(0, m_ssgiDS.get());

        {
            struct SSGIPC { Graphic::PrismaMath::mat4 vp; Graphic::PrismaMath::vec4 cameraPos; Graphic::PrismaMath::vec4 params; };
            SSGIPC pc;
            pc.vp = ctx.camera.projectionMatrix * ctx.camera.viewMatrix;
            pc.cameraPos = {ctx.camera.position.x, ctx.camera.position.y, ctx.camera.position.z, 1.0f};
            pc.params = {16.0f, 0.6f, 5.0f, 0.1f}; // rays=16, intensity=0.6, maxDist=5, stepSize=0.1
            ctx.commandBuffer->PushConstants(Graphic::ShaderType::Unknown, &pc, sizeof(pc));
            ctx.commandBuffer->Draw(3, 1);
        }
        vkCmdEndRenderPass(vkCmd);
    }
    // ====================================================================
    // Phase 3: SwapChain Pass — composite lighting result or debug GBuffer
    // ====================================================================
    ctx.device->BeginSwapChainRenderPass();
    ctx.commandBuffer->SetViewport({0,0,(float)m_width,(float)m_height,0,1});
    ctx.commandBuffer->SetScissorRect({0,0,(int)m_width,(int)m_height});

    int dbgIdx = std::clamp(m_debugGBufferTarget, 0, 4);
    if (m_debugGBufferShow && m_debugGBufferPSO && m_debugGBufferDS[dbgIdx]) {
        ctx.commandBuffer->SetPipelineState(m_debugGBufferPSO.get());
        ctx.commandBuffer->BindDescriptorSet(0, m_debugGBufferDS[dbgIdx].get());
        ctx.commandBuffer->Draw(3, 1);
    } else if (m_compositePSO && m_compositeDS) {
        ctx.commandBuffer->SetPipelineState(m_compositePSO.get());
        ctx.commandBuffer->BindDescriptorSet(0, m_compositeDS.get());
        ctx.commandBuffer->Draw(3, 1);
    } else if (m_forwardPSO) {
        ctx.commandBuffer->SetPipelineState(m_forwardPSO.get());
        int cmdIdx = 0;
        for (const auto& c : cmds) {
            if (!c.mesh) continue;

            Graphic::PrismaMath::mat4 mvp = ctx.camera.projectionMatrix * ctx.camera.viewMatrix * c.transform;

            Graphic::PrismaMath::vec4 baseCol(1.0f, 0.0f, 1.0f, 1);
            const char* matName = "(null)";
            if (c.material) {
                matName = c.material->GetName().c_str();
                const auto* p = c.material->GetParam("BaseColor");
                if (!p) p = c.material->GetParam("basecolor");
                if (p) {
                    if (const auto* col = std::get_if<Prisma::Color>(p))
                        baseCol = {col->r, col->g, col->b, col->a};
                    else if (const auto* v4 = std::get_if<Graphic::PrismaMath::vec4>(p))
                        baseCol = *v4;
                }
            }

            if (!m_firstFrameLogged) {
                LOG_INFO("Deferred3D_Dbg", "Cmd[{}] mat='{}' mesh='{}' color=({:.3},{:.3},{:.3},{:.3})",
                    cmdIdx, matName,
                    c.mesh ? c.mesh->GetName().c_str() : "null",
                    baseCol.r, baseCol.g, baseCol.b, baseCol.a);
            }
            ++cmdIdx;

            struct PushData { Graphic::PrismaMath::mat4 mvp; Graphic::PrismaMath::vec4 color; };
            PushData pd{};
            pd.mvp = mvp;
            pd.color = baseCol;
            ctx.commandBuffer->PushConstants(Graphic::ShaderType::Unknown, &pd, sizeof(pd));

            for (const auto& sm : c.mesh->GetSubMeshes()) {
                if (sm.vertexBuffer && sm.indexBuffer) {
                    ctx.commandBuffer->SetVertexBuffer(sm.vertexBuffer.get(), 0);
                    ctx.commandBuffer->SetIndexBuffer(sm.indexBuffer.get());
                    ctx.commandBuffer->DrawIndexed(sm.indexCount);
                }
            }
        }
    }

    if (!m_firstFrameLogged) {
        LOG_INFO("Deferred3D_Frame1", "GBuffer → Lighting → Composite pipeline active");
        m_firstFrameLogged = true;
    }
}

// Camera adapter impl
void DeferredPipelineAdapter::CameraDataAdapter::SetCameraData(const Graphic::CameraData& d, float w, float h) { m_data = d; m_width = w; m_height = h; }
PrismaMath::mat4 DeferredPipelineAdapter::CameraDataAdapter::GetViewMatrix() const { return m_data.viewMatrix; }
PrismaMath::mat4 DeferredPipelineAdapter::CameraDataAdapter::GetProjectionMatrix() const { return m_data.projectionMatrix; }
PrismaMath::mat4 DeferredPipelineAdapter::CameraDataAdapter::GetViewProjectionMatrix() const { return m_data.projectionMatrix * m_data.viewMatrix; }
PrismaMath::vec3 DeferredPipelineAdapter::CameraDataAdapter::GetPosition() const { return m_data.position; }
PrismaMath::vec3 DeferredPipelineAdapter::CameraDataAdapter::GetForward() const { return {0,0,-1}; }
PrismaMath::vec3 DeferredPipelineAdapter::CameraDataAdapter::GetUp() const { return {0,1,0}; }
PrismaMath::vec3 DeferredPipelineAdapter::CameraDataAdapter::GetRight() const { return {1,0,0}; }
float DeferredPipelineAdapter::CameraDataAdapter::GetFOV() const { return m_data.fov; }
float DeferredPipelineAdapter::CameraDataAdapter::GetNearPlane() const { return m_data.nearPlane; }
float DeferredPipelineAdapter::CameraDataAdapter::GetFarPlane() const { return m_data.farPlane; }
float DeferredPipelineAdapter::CameraDataAdapter::GetAspectRatio() const { return m_width / (m_height > 0 ? m_height : 1); }
void DeferredPipelineAdapter::CameraDataAdapter::SetFOV(float) {}
void DeferredPipelineAdapter::CameraDataAdapter::SetNearFarPlanes(float, float) {}
void DeferredPipelineAdapter::CameraDataAdapter::SetAspectRatio(float) {}
void DeferredPipelineAdapter::CameraDataAdapter::SetViewport(uint32_t w, uint32_t h) { m_width = (float)w; m_height = (float)h; }
void DeferredPipelineAdapter::CameraDataAdapter::Update(Graphic::Timestep) {}
bool DeferredPipelineAdapter::CameraDataAdapter::IsActive() const { return true; }
void DeferredPipelineAdapter::CameraDataAdapter::SetActive(bool) {}
PrismaMath::vec4 DeferredPipelineAdapter::CameraDataAdapter::GetClearColor() const { return m_clearColor; }
void DeferredPipelineAdapter::CameraDataAdapter::SetClearColor(float r, float g, float b, float a) { m_clearColor = {r,g,b,a}; }

void DeferredPipelineAdapter::SetDebugGBufferConfig(int target, bool show)
{
    m_debugGBufferTarget = target;
    m_debugGBufferShow = show;
}

} // namespace Prisma
