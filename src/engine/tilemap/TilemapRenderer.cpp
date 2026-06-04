#include "TilemapRenderer.h"
#include "graphic/Renderer2D.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/ISwapChain.h"
#include "graphic/interfaces/ISampler.h"
#include "graphic/RenderDesc.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "Logger.h"
#include <cmath>
#include <cstring>
#include <vector>

namespace Prisma::Tilemap {

static constexpr uint32_t kQuadVertexStride = 20; // 3*float + 2*float
static const float kQuadVertices[] = {
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
};
static const uint32_t kQuadIndices[] = {
    0, 1, 2, 2, 3, 0
};

TilemapRenderer::~TilemapRenderer() = default;

void TilemapRenderer::Render(const Graphic::OrthographicCamera& camera) {
    if (!m_tilemap || !m_texture) return;

    if (m_useInstancing) {
        BuildInstanceList(camera);
        if (m_instanceCount == 0) return;

        auto* device = Engine::Get().GetRenderSystem()->GetDevice();
        if (!device) return;

        EnsureInstancingResources(device);
        if (!m_instancedPSO || !m_instanceBuffer) return;

        {
            std::vector<TileInstanceData> instanceData(m_instanceCount);
            for (uint32_t i = 0; i < m_instanceCount; i++) {
                instanceData[i].world = m_instanceMatrices[i];
                instanceData[i].uv    = m_instanceUVs[i];
            }
            uint64_t dataSize = static_cast<uint64_t>(m_instanceCount) * sizeof(TileInstanceData);
            m_instanceBuffer->UpdateData(instanceData.data(), dataSize, 0);
        }

        {
            TilemapCameraUBO camUBO;
            camUBO.viewProjection = camera.GetViewProjectionMatrix();
            m_cameraUBO->UpdateData(&camUBO, sizeof(camUBO), 0);
        }

        auto* cmd = device->GetCurrentCommandBuffer();
        if (cmd) {
            RenderInstanced(cmd);
        }
    } else {
        uint32_t tileSize = m_tilemap->GetTileSize();
        float worldTileSize = static_cast<float>(tileSize);

        glm::vec3 camPos = camera.GetPosition();
        float camHalfW = camera.GetAspectRatio() * camera.GetZoom() * 0.5f;
        float camHalfH = camera.GetZoom() * 0.5f;
        float camLeft   = camPos.x - camHalfW;
        float camRight  = camPos.x + camHalfW;
        float camBottom = camPos.y - camHalfH;
        float camTop    = camPos.y + camHalfH;

        camLeft   -= worldTileSize;
        camRight  += worldTileSize;
        camBottom -= worldTileSize;
        camTop    += worldTileSize;

        int startX = std::max(0, static_cast<int>(std::floor(camLeft / worldTileSize)));
        int startY = std::max(0, static_cast<int>(std::floor(camBottom / worldTileSize)));
        int endX = std::min(static_cast<int>(m_tilemap->GetWidth()),
                            static_cast<int>(std::ceil(camRight / worldTileSize)));
        int endY = std::min(static_cast<int>(m_tilemap->GetHeight()),
                            static_cast<int>(std::ceil(camTop / worldTileSize)));

        uint32_t cols = m_tilemap->GetTileSet().GetColumns();
        if (cols == 0) cols = 1;

        for (uint32_t l = 0; l < m_tilemap->GetLayerCount(); l++) {
            const auto* layer = m_tilemap->GetLayer(l);
            if (!layer || !layer->visible) continue;

            for (int y = startY; y < endY; y++) {
                for (int x = startX; x < endX; x++) {
                    uint32_t tileId = layer->GetTile(static_cast<uint32_t>(x),
                                                      static_cast<uint32_t>(y));
                    if (tileId == 0) continue;

                    float wx = static_cast<float>(x) * worldTileSize + worldTileSize * 0.5f;
                    float wy = static_cast<float>(y) * worldTileSize + worldTileSize * 0.5f;

                    const auto* def = m_tilemap->GetTileSet().GetTileDef(tileId);
                    uint32_t texIndex = def ? def->texIndex : 0;
                    uint32_t tx = texIndex % cols;
                    uint32_t ty = texIndex / cols;

                    float u0 = static_cast<float>(tx) / static_cast<float>(cols);
                    float v0 = static_cast<float>(ty) / static_cast<float>(cols);
                    float u1 = u0 + 1.0f / static_cast<float>(cols);
                    float v1 = v0 + 1.0f / static_cast<float>(cols);

                    Vector2 uv[4] = {
                        {u0, v0},
                        {u1, v0},
                        {u1, v1},
                        {u0, v1}
                    };

                    Graphic::Renderer2D::DrawQuad(
                        Vector2(wx, wy),
                        Vector2(worldTileSize, worldTileSize),
                        m_texture,
                        uv,
                        Color(1.0f)
                    );
                }
            }
        }
    }
}

void TilemapRenderer::BuildInstanceList(const Graphic::OrthographicCamera& camera) {
    m_instanceMatrices.clear();
    m_instanceUVs.clear();
    m_instanceCount = 0;

    uint32_t tileSize = m_tilemap->GetTileSize();
    float worldTileSize = static_cast<float>(tileSize);

    glm::vec3 camPos = camera.GetPosition();
    float camHalfW = camera.GetAspectRatio() * camera.GetZoom() * 0.5f;
    float camHalfH = camera.GetZoom() * 0.5f;
    float camLeft   = camPos.x - camHalfW;
    float camRight  = camPos.x + camHalfW;
    float camBottom = camPos.y - camHalfH;
    float camTop    = camPos.y + camHalfH;

    camLeft   -= worldTileSize;
    camRight  += worldTileSize;
    camBottom -= worldTileSize;
    camTop    += worldTileSize;

    int startX = std::max(0, static_cast<int>(std::floor(camLeft / worldTileSize)));
    int startY = std::max(0, static_cast<int>(std::floor(camBottom / worldTileSize)));
    int endX = std::min(static_cast<int>(m_tilemap->GetWidth()),
                        static_cast<int>(std::ceil(camRight / worldTileSize)));
    int endY = std::min(static_cast<int>(m_tilemap->GetHeight()),
                        static_cast<int>(std::ceil(camTop / worldTileSize)));

    uint32_t cols = m_tilemap->GetTileSet().GetColumns();
    if (cols == 0) cols = 1;

    uint32_t estimatedCount = static_cast<uint32_t>((endX - startX) * (endY - startY));
    m_instanceMatrices.reserve(estimatedCount);
    m_instanceUVs.reserve(estimatedCount);

    for (uint32_t l = 0; l < m_tilemap->GetLayerCount(); l++) {
        const auto* layer = m_tilemap->GetLayer(l);
        if (!layer || !layer->visible) continue;

        for (int y = startY; y < endY; y++) {
            for (int x = startX; x < endX; x++) {
                uint32_t tileId = layer->GetTile(static_cast<uint32_t>(x),
                                                  static_cast<uint32_t>(y));
                if (tileId == 0) continue;

                float wx = static_cast<float>(x) * worldTileSize + worldTileSize * 0.5f;
                float wy = static_cast<float>(y) * worldTileSize + worldTileSize * 0.5f;

                Matrix4 world = glm::translate(glm::mat4(1.0f), glm::vec3(wx, wy, 0.0f))
                              * glm::scale(glm::mat4(1.0f), glm::vec3(worldTileSize, worldTileSize, 1.0f));
                m_instanceMatrices.push_back(world);

                const auto* def = m_tilemap->GetTileSet().GetTileDef(tileId);
                uint32_t texIndex = def ? def->texIndex : 0;
                uint32_t tx = texIndex % cols;
                uint32_t ty = texIndex / cols;

                float u0 = static_cast<float>(tx) / static_cast<float>(cols);
                float v0 = static_cast<float>(ty) / static_cast<float>(cols);
                float u1 = u0 + 1.0f / static_cast<float>(cols);
                float v1 = v0 + 1.0f / static_cast<float>(cols);

                m_instanceUVs.emplace_back(u0, v0, u1, v1);
            }
        }
    }

    m_instanceCount = static_cast<uint32_t>(m_instanceMatrices.size());
}

void TilemapRenderer::EnsureInstancingResources(Graphic::IRenderDevice* device) {
    if (m_instancedPSO) return;

    auto* factory = device->GetResourceFactory();
    auto* rm = Engine::Get().GetRenderResourceManager();
    if (!factory || !rm) {
        LOG_ERROR("TilemapRenderer", "No resource factory or resource manager available");
        return;
    }

    {
        Graphic::BufferDesc desc;
        desc.name = "Tilemap_QuadVerts";
        desc.type = Graphic::BufferType::Vertex;
        desc.size = 4 * kQuadVertexStride;
        desc.stride = kQuadVertexStride;
        desc.initialData = kQuadVertices;
        desc.usage = Graphic::BufferUsage::Immutable;
        auto buf = factory->CreateBufferImpl(desc);
        if (!buf) {
            LOG_ERROR("TilemapRenderer", "Failed to create quad vertex buffer");
            return;
        }
        m_quadVertexBuffer.reset(buf.release());
    }

    {
        Graphic::BufferDesc desc;
        desc.name = "Tilemap_QuadIndices";
        desc.type = Graphic::BufferType::Index;
        desc.size = 6 * sizeof(uint32_t);
        desc.initialData = kQuadIndices;
        desc.usage = Graphic::BufferUsage::Immutable;
        auto buf = factory->CreateBufferImpl(desc);
        if (!buf) {
            LOG_ERROR("TilemapRenderer", "Failed to create quad index buffer");
            return;
        }
        m_quadIndexBuffer.reset(buf.release());
    }

    {
        Graphic::BufferDesc desc;
        desc.name = "Tilemap_InstanceData";
        desc.type = Graphic::BufferType::Vertex;
        desc.size = static_cast<uint64_t>(kMaxTileInstances) * sizeof(TileInstanceData);
        desc.stride = sizeof(TileInstanceData);
        desc.usage = Graphic::BufferUsage::Dynamic;
        auto buf = factory->CreateBufferImpl(desc);
        if (!buf) {
            LOG_ERROR("TilemapRenderer", "Failed to create instance buffer");
            return;
        }
        m_instanceBuffer.reset(buf.release());
    }

    {
        Graphic::BufferDesc desc;
        desc.name = "Tilemap_CameraUBO";
        desc.type = Graphic::BufferType::Constant;
        desc.size = sizeof(TilemapCameraUBO);
        desc.usage = Graphic::BufferUsage::Dynamic;
        auto buf = factory->CreateBufferImpl(desc);
        if (!buf) {
            LOG_ERROR("TilemapRenderer", "Failed to create camera UBO");
            return;
        }
        m_cameraUBO.reset(buf.release());
    }

    m_instancedVertShader = rm->LoadShaderSync("assets/shaders/2d/TilemapInstanced.vert.spv", "main");
    if (!m_instancedVertShader) {
        LOG_ERROR("TilemapRenderer", "Failed to load vertex shader");
        return;
    }
    m_instancedFragShader = rm->LoadShaderSync("assets/shaders/2d/TilemapInstanced.frag.spv", "main");
    if (!m_instancedFragShader) {
        LOG_ERROR("TilemapRenderer", "Failed to load fragment shader");
        return;
    }

    m_instancedPSO.reset(factory->CreatePipelineStateImpl().release());
    m_instancedPSO->SetShader(Graphic::ShaderType::Vertex, m_instancedVertShader);
    m_instancedPSO->SetShader(Graphic::ShaderType::Pixel, m_instancedFragShader);
    m_instancedPSO->SetPrimitiveTopology(Graphic::PrimitiveTopology::TriangleList);

    {
        std::vector<Graphic::VertexInputAttribute> attrs;
        attrs.reserve(7);
        attrs.push_back({"POSITION", 0, Graphic::TextureFormat::RGB32_Float, 0, 0, 0, 0});
        attrs.push_back({"TEXCOORD", 0, Graphic::TextureFormat::RG32_Float, 0, 12, 0, 0});
        attrs.push_back({"WORLD", 0, Graphic::TextureFormat::RGBA32_Float, 1, 0, 1, 1});
        attrs.push_back({"WORLD", 1, Graphic::TextureFormat::RGBA32_Float, 1, 16, 1, 1});
        attrs.push_back({"WORLD", 2, Graphic::TextureFormat::RGBA32_Float, 1, 32, 1, 1});
        attrs.push_back({"WORLD", 3, Graphic::TextureFormat::RGBA32_Float, 1, 48, 1, 1});
        attrs.push_back({"TEXCOORD", 1, Graphic::TextureFormat::RGBA32_Float, 1, 64, 1, 1});
        m_instancedPSO->SetInputLayout(attrs);
    }

    {
        Graphic::TextureFormat rtFormat = Graphic::TextureFormat::RGBA8_UNorm;
        Graphic::TextureFormat dsFormat = Graphic::TextureFormat::D32_Float;
        if (auto* swapchain = device->GetSwapChain()) {
            rtFormat = swapchain->GetFormat();
        }
        m_instancedPSO->SetRenderTargetFormats({rtFormat});
        m_instancedPSO->SetDepthStencilFormat(dsFormat);
    }

    {
        Graphic::BlendState blend;
        blend.blendEnable = true;
        blend.srcBlend = Graphic::BlendFactorType::SrcAlpha;
        blend.destBlend = Graphic::BlendFactorType::InvSrcAlpha;
        blend.srcBlendAlpha = Graphic::BlendFactorType::One;
        blend.destBlendAlpha = Graphic::BlendFactorType::Zero;
        m_instancedPSO->SetBlendState(blend);
    }

    {
        Graphic::RasterizerState raster;
        raster.cullEnable = false;
        raster.cullMode = Graphic::CullMode::None;
        raster.fillMode = Graphic::FillMode::Solid;
        raster.depthClipEnable = true;
        m_instancedPSO->SetRasterizerState(raster);
    }

    {
        Graphic::DepthStencilState depth;
        depth.depthEnable = true;
        depth.depthWriteEnable = false;
        depth.depthFunc = Graphic::ComparisonFunc::Less;
        m_instancedPSO->SetDepthStencilState(depth);
    }

    if (!m_instancedPSO->Create(device)) {
        LOG_ERROR("TilemapRenderer", "Failed to create instanced PSO: {}",
                  m_instancedPSO->GetErrors());
        m_instancedPSO.reset();
        return;
    }

    auto layouts = m_instancedPSO->GetDescriptorSetLayouts();
    if (layouts.empty()) {
        LOG_ERROR("TilemapRenderer", "PSO has no descriptor set layouts");
        m_instancedPSO.reset();
        return;
    }

    m_instancedDescSetLayout = layouts[0];
    m_instancedDescSet = factory->CreateDescriptorSet(m_instancedDescSetLayout.get());
    if (!m_instancedDescSet) {
        LOG_ERROR("TilemapRenderer", "Failed to create descriptor set");
        m_instancedPSO.reset();
        return;
    }

    auto sampler = rm->GetDefaultSampler();
    m_instancedDescSet->BindTexture(0, m_texture.get(), sampler ? sampler.get() : nullptr);
    m_instancedDescSet->BindBuffer(1, m_cameraUBO.get(), 0, sizeof(TilemapCameraUBO),
                                   Graphic::DescriptorType::UniformBuffer);
    m_instancedDescSet->Update();

    LOG_INFO("TilemapRenderer", "GPU instancing resources created");
}

void TilemapRenderer::RenderInstanced(Graphic::ICommandBuffer* cmd) {
    if (!cmd || m_instanceCount == 0) return;
    if (!m_instancedPSO || !m_instanceBuffer) return;
    if (!m_quadVertexBuffer || !m_quadIndexBuffer) return;

    cmd->SetPipelineState(m_instancedPSO.get());
    cmd->BindDescriptorSet(0, m_instancedDescSet.get());
    cmd->SetVertexBuffer(m_quadVertexBuffer.get(), 0, 0);
    cmd->SetVertexBuffer(m_instanceBuffer.get(), 1, 0);
    cmd->SetIndexBuffer(m_quadIndexBuffer.get(), false, 0);
    cmd->DrawIndexed(6, m_instanceCount, 0);
}

}
