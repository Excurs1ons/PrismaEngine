#include "Light2DPass.h"
#include "LightManager2D.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/RenderCommandContext.h"
#include "adapters/vulkan/VulkanCommandBuffer.h"
#include "adapters/vulkan/VulkanPipelineState.h"
#include "adapters/vulkan/VulkanResources.h"
#include "Logger.h"
#include <cmath>

namespace Prisma::Graphic {

// 静态阴影投射体数据
std::vector<ShadowCaster2D> Light2DPass::s_shadowCasters;
std::vector<bool> Light2DPass::s_casterActive;

// 阴影推流常量（必须与 GLSL 布局匹配）
struct ShadowPushConstants {
    alignas(16) Matrix4 u_MVP;
    float u_ShadowIntensity;
};

/**
 * @brief 内部渲染目标代理
 */
class LightTextureRTProxy final : public ITextureRenderTarget {
public:
    LightTextureRTProxy(ITexture* texture) : m_texture(texture) {}
    uint32_t GetWidth() const override { return m_texture ? static_cast<uint32_t>(m_texture->GetWidth()) : 0; }
    uint32_t GetHeight() const override { return m_texture ? static_cast<uint32_t>(m_texture->GetHeight()) : 0; }
    TextureFormat GetFormat() const override { return m_texture ? m_texture->GetFormat() : TextureFormat::Unknown; }
    TextureType GetType() const override { return m_texture ? m_texture->GetTextureType() : TextureType::Texture2D; }
    
    void* GetNativeHandle() const override { return nullptr; }

    bool IsSwapChain() const override { return false; }
    void Clear(const float color[4]) override {
        if (m_texture) {
            m_texture->Clear(Color(color[0], color[1], color[2], color[3]));
        }
    }

    uint32_t GetMipLevels() const override { return m_texture ? m_texture->GetMipLevels() : 0; }
    uint32_t GetArraySize() const override { return m_texture ? m_texture->GetArraySize() : 0; }
    ITexture* GetTexture() override { return m_texture; }

private:
    ITexture* m_texture;
};

struct LightPushConstants {
    alignas(8) Vector2 position;
    alignas(16) Vector3 color;
    float intensity;
    float radius;
    float falloff;
    alignas(16) Matrix4 invVP;
};

// 注：此 Pass 使用 invVP 仅为了从屏幕 UV 反推世界坐标算光照距离，
// 并非 3D 前向渲染。对 ortho 2D 来说可简化为 scale+translate，暂保留现有实现。
Light2DPass::Light2DPass() : Pass2D("Light2DPass") {
    m_priority = 120;
}

Light2DPass::~Light2DPass() {}

void Light2DPass::Update(Prisma::Timestep ts) {
    UpdateTime(ts);
}

void Light2DPass::Execute(const PassExecutionContext& /*context*/) {
}

// ========== 阴影投射体静态注册接口 ==========

uint32_t Light2DPass::RegisterCaster(const ShadowCaster2D& caster) {
    for (size_t i = 0; i < s_casterActive.size(); ++i) {
        if (!s_casterActive[i]) {
            s_shadowCasters[i] = caster;
            s_casterActive[i] = true;
            return static_cast<uint32_t>(i);
        }
    }
    s_shadowCasters.push_back(caster);
    s_casterActive.push_back(true);
    return static_cast<uint32_t>(s_shadowCasters.size() - 1);
}

void Light2DPass::RemoveCaster(uint32_t handle) {
    if (handle < s_casterActive.size()) {
        s_casterActive[handle] = false;
    }
}

void Light2DPass::ClearCasters() {
    s_shadowCasters.clear();
    s_casterActive.clear();
}

const std::vector<ShadowCaster2D>& Light2DPass::GetCasters() {
    return s_shadowCasters;
}

// ========== 阴影渲染 ==========

void Light2DPass::EnsureShadowResources(IRenderDevice* device) {
    if (m_shadowPSO) return;

    auto rf = device->GetResourceFactory();
    auto rm = Engine::Get().GetRenderResourceManager();

    // 1. 加载阴影着色器
    m_shadowVertShader = rm->LoadShaderSync("assets/shaders/ShadowGeometry.vert.spv");
    m_shadowFragShader = rm->LoadShaderSync("assets/shaders/ShadowGeometry.frag.spv");

    if (!m_shadowVertShader || !m_shadowFragShader) {
        LOG_ERROR("Light2DPass", "无法加载 2D 阴影着色器");
        return;
    }

    // 2. 创建动态顶点缓冲区（初始容量 1024 顶点，按需增长）
    uint64_t initialSize = 1024 * sizeof(Vector2);
    BufferDesc vbDesc;
    vbDesc.type = BufferType::Vertex;
    vbDesc.size = initialSize;
    vbDesc.usage = BufferUsage::Dynamic;
    vbDesc.stride = sizeof(Vector2);

    auto buf = rf->CreateBufferImpl(vbDesc);
    if (buf) {
        buf->SetDebugName("Light2DPass_ShadowVB");
        m_shadowVB = std::shared_ptr<IBuffer>(std::move(buf));
        m_shadowVBCapacity = 1024;
    }

    // 3. 创建阴影 PSO
    auto pso = rf->CreatePipelineStateImpl();
    pso->SetShader(ShaderType::Vertex, m_shadowVertShader);
    pso->SetShader(ShaderType::Pixel, m_shadowFragShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleStrip);

    // 顶点输入：vec2 位置
    VertexInputAttribute attr;
    attr.semanticName = "POSITION";
    attr.format = TextureFormat::RG32_Float;
    attr.inputSlot = 0;
    attr.alignedByteOffset = 0;
    pso->SetInputLayout({ attr });

    // 阴影混合: Src=Zero, Dst=InvSrcAlpha, Op=Add
    // dest.rgb = dest.rgb * (1 - srcAlpha)
    BlendState bs;
    bs.blendEnable = true;
    bs.srcBlend = BlendFactorType::Zero;
    bs.destBlend = BlendFactorType::InvSrcAlpha;
    bs.blendOp = BlendOp::Add;
    bs.srcBlendAlpha = BlendFactorType::Zero;
    bs.destBlendAlpha = BlendFactorType::InvSrcAlpha;
    bs.blendOpAlpha = BlendOp::Add;
    pso->SetBlendState(bs);

    DepthStencilState ds;
    ds.depthEnable = false;
    ds.depthWriteEnable = false;
    pso->SetDepthStencilState(ds);

    RasterizerState rs;
    rs.cullEnable = false;
    pso->SetRasterizerState(rs);

    pso->SetRenderTargetFormat(0, TextureFormat::RGBA16_Float);

    // 复用光照纹理的离屏 RenderPass
    auto* vkTex = dynamic_cast<Vulkan::VulkanTexture*>(m_lightTexture.get());
    if (vkTex) {
        VkRenderPass shadowRP = Vulkan::VulkanCommandBuffer::PreCreateOffscreenRenderPass(
            vkTex->GetVkDevice(),
            vkTex->GetVkImageView(), vkTex->GetVkFormat(),
            VK_NULL_HANDLE, VK_FORMAT_UNDEFINED,
            m_width, m_height, true, false);
        auto* vkPSO = dynamic_cast<Vulkan::VulkanPipelineState*>(pso.get());
        if (vkPSO && shadowRP != VK_NULL_HANDLE) {
            vkPSO->SetCustomRenderPass(shadowRP);
        }
    }

    if (pso->Create(device)) {
        m_shadowPSO = std::shared_ptr<IPipelineState>(std::move(pso));
    } else {
        LOG_ERROR("Light2DPass", "创建阴影 PSO 失败: {}", pso->GetErrors());
    }
}

void Light2DPass::RenderShadowGeometry(ICommandBuffer* cmd, IRenderDevice* device,
                                        const Matrix4& vp, const Vector2& lightPos,
                                        float lightRadius, float shadowIntensity) {
    if (!m_shadowPSO || !m_shadowVB) return;

    // 收集当前帧所有可见的阴影投射体（在光源半径内）
    // 每最大 4 个顶点组成一个 shadow quad（triangle strip）
    std::vector<Vector2> vertices;
    vertices.reserve(MAX_SHADOW_VERTS);

    for (size_t ci = 0; ci < s_shadowCasters.size(); ++ci) {
        if (!s_casterActive[ci]) continue;
        const auto& caster = s_shadowCasters[ci];
        Vector2 cMin = caster.GetMin();
        Vector2 cMax = caster.GetMax();

        // 快速 AABB ↔ 光源圆裁剪测试：
        // 找到 AABB 上距离光源最近的点
        float closestX = std::max(cMin.x, std::min(lightPos.x, cMax.x));
        float closestY = std::max(cMin.y, std::min(lightPos.y, cMax.y));
        float dx = lightPos.x - closestX;
        float dy = lightPos.y - closestY;
        float distSq = dx * dx + dy * dy;
        if (distSq > lightRadius * lightRadius) continue;

        // 定义 4 条边：每条边的两个端点 + 向外法线
        struct EdgeDef { Vector2 v0, v1; Vector2 normal; };
        EdgeDef edges[4] = {
            {{cMin.x, cMin.y}, {cMax.x, cMin.y}, {0.0f, -1.0f}},  // 底边
            {{cMax.x, cMin.y}, {cMax.x, cMax.y}, {1.0f,  0.0f}},  // 右边
            {{cMax.x, cMax.y}, {cMin.x, cMax.y}, {0.0f,  1.0f}},  // 顶边
            {{cMin.x, cMax.y}, {cMin.x, cMin.y}, {-1.0f, 0.0f}},  // 左边
        };

        for (const auto& edge : edges) {
            Vector2 mid = (edge.v0 + edge.v1) * 0.5f;
            Vector2 toLight = lightPos - mid;
            float dot = edge.normal.x * toLight.x + edge.normal.y * toLight.y;
            if (dot <= 0.0f) continue; // 背向光源的面不产生阴影

            // 计算 extrusion 方向
            Vector2 dir0 = edge.v0 - lightPos;
            Vector2 dir1 = edge.v1 - lightPos;
            float len0 = std::sqrt(dir0.x * dir0.x + dir0.y * dir0.y);
            float len1 = std::sqrt(dir1.x * dir1.x + dir1.y * dir1.y);

            // 防止除零
            if (len0 < 1e-6f || len1 < 1e-6f) continue;

            Vector2 ext0 = dir0 / len0 * lightRadius;
            Vector2 ext1 = dir1 / len1 * lightRadius;

            // Triangle strip: V0, V1, V3, V2
            //  → v0 和 v1 是边缘的起始顶点
            //  → v3 = v0 + ext0, v2 = v1 + ext1
            Vector2 p0 = edge.v0;
            Vector2 p1 = edge.v1;
            Vector2 p2 = p1 + ext1;
            Vector2 p3 = p0 + ext0;

            vertices.push_back(p0);
            vertices.push_back(p1);
            vertices.push_back(p3);
            vertices.push_back(p2);

            if (vertices.size() >= MAX_SHADOW_VERTS) goto flush_shadow;
        }
    }

flush_shadow:
    if (vertices.empty()) return;

    uint32_t vertCount = static_cast<uint32_t>(vertices.size());

    // 确保顶点缓冲区足够大
    uint64_t neededSize = vertCount * sizeof(Vector2);
    if (neededSize > m_shadowVBCapacity * sizeof(Vector2)) {
        // 按需增长（2 倍）
        uint32_t newCapacity = std::max(m_shadowVBCapacity * 2, vertCount);
        BufferDesc vbDesc;
        vbDesc.type = BufferType::Vertex;
        vbDesc.size = newCapacity * sizeof(Vector2);
        vbDesc.usage = BufferUsage::Dynamic;
        vbDesc.stride = sizeof(Vector2);
        auto rf = device->GetResourceFactory();
        auto buf = rf->CreateBufferImpl(vbDesc);
        if (buf) {
            buf->SetDebugName("Light2DPass_ShadowVB");
            m_shadowVB = std::shared_ptr<IBuffer>(std::move(buf));
            m_shadowVBCapacity = newCapacity;
        } else {
            LOG_ERROR("Light2DPass", "无法扩展阴影顶点缓冲区");
            return;
        }
    }

    // 上传顶点数据
    m_shadowVB->UpdateData(vertices.data(), neededSize, 0);

    // 设置推流常量
    ShadowPushConstants pc;
    pc.u_MVP = vp;
    pc.u_ShadowIntensity = shadowIntensity;

    // 绘制阴影几何
    cmd->SetPipelineState(m_shadowPSO.get());
    cmd->SetVertexBuffer(m_shadowVB.get(), 0);
    cmd->PushConstants(ShaderType::VertexAndPixel, &pc, sizeof(pc));
    cmd->Draw(vertCount, 1, 0);
}

void Light2DPass::ExecuteLight(ICommandBuffer* cmd, IRenderDevice* device, uint32_t width, uint32_t height) {
    if (!cmd || !device) return;

    EnsureResources(width, height, device);
    if (!m_pointLightPSO) return;

    // 1. 开始渲染到光照纹理
    // 清除色 = 环境光（由 LightManager2D 全局设置，可通过 C# 组件调节）
    //   (1,1,1) = 完全照亮（无光照区域 sprite 正常可见）
    //   (0,0,0) = 无环境光（只有光源区域可见）
    RenderPassDesc rpDesc;
    rpDesc.renderTarget = m_lightTexture.get();
    {
        const auto& ambient = LightManager2D::Get().GetAmbientColor();
        rpDesc.clearColor = { ambient.x, ambient.y, ambient.z, 0.0f };
    }
    rpDesc.clearRenderTarget = true;
    rpDesc.renderArea = { 0, 0, (int)width, (int)height };
    
    cmd->BeginRenderPass(rpDesc);
    cmd->SetViewport({ 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f });
    cmd->SetScissorRect({ 0, 0, (int)width, (int)height });

    cmd->SetPipelineState(m_pointLightPSO.get());
    
    // 2. 遍历所有光源并绘制
    const auto& lights = LightManager2D::Get().GetLights();
    Matrix4 invVP = glm::inverse(m_viewProjection);
    
    for (const auto& light : lights) {
        if (!light || !light->IsPoint()) continue;
        
        LightPushConstants pc;
        pc.position = light->GetPosition();
        pc.color = light->GetColor();
        pc.intensity = light->GetIntensity();
        pc.radius = light->GetRadius();
        pc.falloff = light->GetFalloffCurve();
        pc.invVP = invVP;
        
        cmd->PushConstants(ShaderType::VertexAndPixel, &pc, sizeof(pc));
        cmd->Draw(3, 1, 0); // 全屏三角形技巧
    }

    // 3. 渲染阴影几何体（对每个投射阴影的光源）
    EnsureShadowResources(device);
    if (m_shadowPSO && !s_shadowCasters.empty()) {
        for (const auto& light : lights) {
            if (!light || !light->IsPoint()) continue;
            if (!light->IsCastShadows()) continue;
            if (light->GetIntensity() <= 0.0f) continue;

            float shadowIntensity = light->GetShadowSoftness() * 0.5f;
            RenderShadowGeometry(cmd, device, m_viewProjection,
                                 light->GetPosition(), light->GetRadius(),
                                 shadowIntensity);
        }
    }
    
    cmd->EndRenderPass();
}

void Light2DPass::EnsureResources(uint32_t width, uint32_t height, IRenderDevice* device) {
    if (m_width == width && m_height == height && m_lightTexture && m_pointLightPSO) return;

    m_width = width;
    m_height = height;
    
    auto rf = device->GetResourceFactory();
    auto rm = Engine::Get().GetRenderResourceManager();
    
    // 1. 创建光照纹理
    if (!m_lightTexture || m_lightTexture->GetWidth() != (float)width || m_lightTexture->GetHeight() != (float)height) {
        TextureDesc desc;
        desc.width = width;
        desc.height = height;
        desc.format = TextureFormat::RGBA16_Float; // HDR 支持
        desc.allowRenderTarget = true;
        desc.allowShaderResource = true;
        
        m_lightTexture = rf->CreateTextureImpl(desc);
        m_lightRT = std::make_shared<LightTextureRTProxy>(m_lightTexture.get());
    }

    // 2. 加载着色器并创建管线状态
    if (!m_pointLightPSO) {
        m_pointLightVertexShader = rm->LoadShaderSync("assets/shaders/PointLight2D.vert.spv");
        m_pointLightPixelShader = rm->LoadShaderSync("assets/shaders/PointLight2D.frag.spv");
        
        if (m_pointLightVertexShader && m_pointLightPixelShader) {
            // 为光照纹理格式创建自定义 RenderPass（RGBA16_Float ≠ 交换链 B8G8R8A8）
            auto* vkTex = dynamic_cast<Vulkan::VulkanTexture*>(m_lightTexture.get());
            VkRenderPass lightRP = VK_NULL_HANDLE;
            if (vkTex) {
                lightRP = Vulkan::VulkanCommandBuffer::PreCreateOffscreenRenderPass(
                    vkTex->GetVkDevice(),
                    vkTex->GetVkImageView(), vkTex->GetVkFormat(),
                    VK_NULL_HANDLE, VK_FORMAT_UNDEFINED,
                    width, height, true, false);
            }

            auto pso = rf->CreatePipelineStateImpl();
            pso->SetShader(ShaderType::Vertex, m_pointLightVertexShader);
            pso->SetShader(ShaderType::Pixel, m_pointLightPixelShader);
            pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);

            // Additive Blending: SrcColor * 1 + DestColor * 1
            BlendState bs;
            bs.blendEnable = true;
            bs.srcBlend = BlendFactorType::One;
            bs.destBlend = BlendFactorType::One;
            bs.blendOp = BlendOp::Add;
            pso->SetBlendState(bs);

            DepthStencilState ds;
            ds.depthEnable = false;
            ds.depthWriteEnable = false;
            pso->SetDepthStencilState(ds);

            RasterizerState rs;
            rs.cullEnable = false;
            pso->SetRasterizerState(rs);

            pso->SetRenderTargetFormat(0, TextureFormat::RGBA16_Float);
            pso->SetInputLayout({});  // 全屏三角形（vertex_id），无顶点输入

            // 设置离屏 RenderPass，确保格式兼容
            auto* vkPSO = dynamic_cast<Vulkan::VulkanPipelineState*>(pso.get());
            if (vkPSO && lightRP != VK_NULL_HANDLE) {
                vkPSO->SetCustomRenderPass(lightRP);
            }

            if (pso->Create(device)) {
                m_pointLightPSO = std::shared_ptr<IPipelineState>(std::move(pso));
            } else {
                LOG_ERROR("Light2DPass", "创建光照 PSO 失败: {}", pso->GetErrors());
            }
        } else {
            LOG_ERROR("Light2DPass", "无法加载 2D 光照着色器");
        }
    }
}

} // namespace Prisma::Graphic
