#include "OpaquePass.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/Mesh.h"
#include "graphic/Material.h"
#include "Logger.h"

namespace Prisma::Graphic {

OpaquePass::OpaquePass() : ForwardRenderPass("OpaquePass") {}

void OpaquePass::SetLights(const std::vector<Light>& lights) {
    m_Lights = lights;
}

void OpaquePass::Update(Prisma::Timestep ts) {
    ForwardRenderPass::Update(ts);
}

void OpaquePass::Execute(const PassExecutionContext& context) {
    // New IPass implementation
}

void OpaquePass::Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands) {
    if (!cmd || commands.empty()) return;

    Material* lastMaterial = nullptr;
    Shader* lastShader = nullptr;

    for (const auto& command : commands) {
        if (!command.mesh || !command.material) continue;

        Shader* currentShader = command.material->GetShader().get();
        if (currentShader != lastShader) {
            lastShader = currentShader;
        }

        if (command.material != lastMaterial) {
            command.material->Bind(cmd);
            lastMaterial = command.material;
        }

        cmd->PushConstants(ShaderType::Vertex, &command.transform, sizeof(PrismaMath::mat4));

        for (const auto& subMesh : command.mesh->GetSubMeshes()) {
            if (subMesh.vertexBuffer && subMesh.indexBuffer) {
                cmd->SetVertexBuffer(subMesh.vertexBuffer.get(), 0);
                cmd->SetIndexBuffer(subMesh.indexBuffer.get());
                cmd->DrawIndexed(subMesh.indexCount);
            }
        }
    }
}

} // namespace Prisma::Graphic
