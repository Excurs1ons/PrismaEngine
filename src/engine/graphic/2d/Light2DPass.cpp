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
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/RenderCommandContext.h"
#include "Logger.h"

namespace Prisma::Graphic {

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

Light2DPass::Light2DPass() : ForwardRenderPass("Light2DPass") {
    m_priority = 120;
}

Light2DPass::~Light2DPass() {}

void Light2DPass::Update(Prisma::Timestep ts) {
    UpdateTime(ts);
}

void Light2DPass::Execute(const PassExecutionContext& context) {
    // 基础 Execute 逻辑，可以用于非 CommandBuffer 驱动的渲染 (如需要)
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
        
        cmd->PushConstants(ShaderType::Pixel, &pc, sizeof(pc));
        cmd->Draw(3, 1, 0); // 全屏三角形技巧
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
            PipelineStateDesc desc;
            desc.vertexShader = m_pointLightVertexShader;
            desc.pixelShader = m_pointLightPixelShader;
            desc.primitiveTopology = PrimitiveTopology::TriangleList;
            
            // Additive Blending: SrcColor * 1 + DestColor * 1
            desc.blendState.blendEnable = true;
            desc.blendState.srcBlend = BlendFactorType::One;
            desc.blendState.destBlend = BlendFactorType::One;
            desc.blendState.blendOp = BlendOp::Add;
            
            desc.depthStencilState.depthEnable = false;
            desc.depthStencilState.depthWriteEnable = false;
            desc.rasterizerState.cullEnable = false;
            
            desc.renderTargetFormats[0] = TextureFormat::RGBA16_Float;
            desc.numRenderTargets = 1;
            
            m_pointLightPSO = rm->CreatePipelineState(desc);
        } else {
            LOG_ERROR("Light2DPass", "无法加载 2D 光照着色器");
        }
    }
}

} // namespace Prisma::Graphic
