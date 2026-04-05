#include "OpaquePass.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/Mesh.h"
#include "graphic/Material.h"
#include "graphic/Shader.h"
#include "graphic/interfaces/ISwapChain.h"
#include "logger/Logger.h"
#include <fstream>
#include <iterator>

namespace Prisma::Graphic {

OpaquePass::OpaquePass() : ForwardRenderPass("OpaquePass") {}

namespace {
struct alignas(16) QuadPushConstants {
    PrismaMath::mat4 mvp;
    Prisma::Color color;
};
}

void OpaquePass::SetLights(const std::vector<Light>& lights) {
    m_Lights = lights;
}

void OpaquePass::Update(Prisma::Timestep ts) {
    ForwardRenderPass::Update(ts);
}

void OpaquePass::Execute(const PassExecutionContext& context) {
    if (!context.deviceContext) {
        return;
    }

    if (context.renderTarget && context.depthStencil) {
        context.deviceContext->SetRenderTarget(context.renderTarget, context.depthStencil);
    } else if (context.renderTarget) {
        context.deviceContext->SetRenderTarget(context.renderTarget);
    }

    if (context.sceneData) {
        context.deviceContext->SetViewport(
            0.0f,
            0.0f,
            static_cast<float>(context.sceneData->viewport.width),
            static_cast<float>(context.sceneData->viewport.height)
        );
    }

    context.deviceContext->MemoryBarrier();
}

void OpaquePass::Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands) {
    if (!cmd || commands.empty() || !m_device) return;
    if (!EnsureDefaultPipeline()) return;

    cmd->SetPipelineState(m_defaultPipelineState.get());
    float width = 1.0f;
    float height = 1.0f;
    if (auto* swapChain = m_device->GetSwapChain()) {
        width = static_cast<float>(swapChain->GetWidth());
        height = static_cast<float>(swapChain->GetHeight());
    }
    cmd->SetViewport(Viewport{0.0f, 0.0f, width, height, 0.0f, 1.0f});
    cmd->SetScissorRect(Rect{0, 0, static_cast<int>(width), static_cast<int>(height)});

    for (const auto& command : commands) {
        if (!command.mesh) continue;

        if (command.material) {
            command.material->Bind(cmd);
        }

        QuadPushConstants pushConstants{};
        pushConstants.mvp = m_projection * m_view * command.transform;
        pushConstants.color = command.color;
        cmd->PushConstants(ShaderType::Vertex, &pushConstants, sizeof(pushConstants));
        cmd->PushConstants(ShaderType::Pixel, &pushConstants, sizeof(pushConstants));

        for (const auto& subMesh : command.mesh->GetSubMeshes()) {
            if (subMesh.vertexBuffer && subMesh.indexBuffer) {
                cmd->SetVertexBuffer(subMesh.vertexBuffer.get(), 0);
                cmd->SetIndexBuffer(subMesh.indexBuffer.get());
                cmd->DrawIndexed(subMesh.indexCount);
            }
        }
    }
}

bool OpaquePass::EnsureDefaultPipeline() {
    if (m_defaultPipelineState) {
        return true;
    }
    if (!m_device || !m_device->GetResourceFactory()) {
        return false;
    }

    auto loadShader = [this](const char* path, ShaderType type) -> std::shared_ptr<IShader> {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return nullptr;
        }

        std::vector<uint8_t> bytecode((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (bytecode.empty()) {
            return nullptr;
        }

        ShaderDesc desc;
        desc.filename = path;
        desc.entryPoint = "main";
        desc.language = ShaderLanguage::SPIRV;
        desc.type = type;

        ShaderReflection reflection;
        if (type == ShaderType::Pixel) {
            ShaderResource albedoMap;
            albedoMap.Name = "u_AlbedoMap";
            albedoMap.ResourceType = ShaderResource::Type::Sampler2D;
            albedoMap.Binding = 0;
            albedoMap.Set = 0;
            reflection.Resources.push_back(albedoMap);
        }

        auto shader = m_device->GetResourceFactory()->CreateShaderImpl(desc, bytecode, reflection);
        return shader ? std::shared_ptr<IShader>(std::move(shader)) : nullptr;
    };

    m_defaultVertexShader = loadShader("assets/shaders/Renderer2D.vert.spv", ShaderType::Vertex);
    m_defaultPixelShader = loadShader("assets/shaders/Renderer2D.frag.spv", ShaderType::Pixel);
    if (!m_defaultVertexShader || !m_defaultPixelShader) {
        LOG_ERROR("OpaquePass", "无法加载 Renderer2D SPIR-V 着色器。");
        return false;
    }

    auto pso = m_device->GetResourceFactory()->CreatePipelineStateImpl();
    if (!pso) {
        return false;
    }

    pso->SetShader(ShaderType::Vertex, m_defaultVertexShader);
    pso->SetShader(ShaderType::Pixel, m_defaultPixelShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    if (!pso->Create(m_device)) {
        LOG_ERROR("OpaquePass", "创建 Renderer2D 管线失败: {0}", pso->GetErrors());
        return false;
    }

    m_defaultPipelineState = std::shared_ptr<IPipelineState>(std::move(pso));
    return true;
}

} // namespace Prisma::Graphic
