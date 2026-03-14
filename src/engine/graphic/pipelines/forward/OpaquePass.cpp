#include "OpaquePass.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IDescriptorSet.h" // 包含描述符接口
#include "graphic/Mesh.h"
#include "graphic/Material.h"
#include "Logger.h"

namespace Prisma::Graphic {

OpaquePass::OpaquePass() 
    : m_ViewMatrix(1.0f), m_ProjMatrix(1.0f) {}

void OpaquePass::SetCameraData(const PrismaMath::mat4& view, const PrismaMath::mat4& proj) {
    m_ViewMatrix = view;
    m_ProjMatrix = proj;
}

void OpaquePass::SetLights(const std::vector<Light>& lights) {
    m_Lights = lights;
}

void OpaquePass::Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands) {
    if (!cmd || commands.empty()) return;

    // 1. 准备全局数据 (Set 0)
    // 在真实 RHI 中，这里会向 RenderDevice 请求一个临时的 DescriptorSet。
    // 为了演示，我们假设已经拿到了一个全局 Set。
    // IDescriptorSet* globalSet = GetGlobalDescriptorSet(); 
    // globalSet->BindBuffer(0, cameraBuffer);
    // globalSet->Update();
    // cmd->BindDescriptorSet(0, globalSet);

    Material* lastMaterial = nullptr;
    Shader* lastShader = nullptr;

    for (const auto& command : commands) {
        if (!command.mesh || !command.material) continue;

        // 2. 状态切换优化：Shader 切换
        Shader* currentShader = command.material->GetShader().get();
        if (currentShader != lastShader) {
            // cmd->SetPipelineState(currentShader->GetPipeline());
            lastShader = currentShader;
        }

        // 3. 状态切换优化：Material 切换 (Set 1)
        if (command.material != lastMaterial) {
            command.material->Bind(cmd); // 内部会调用 cmd->BindDescriptorSet(1, ...)
            lastMaterial = command.material;
        }

        // 4. 提交物体的变换矩阵 (Push Constants)
        // 这是最快的方式，不需要 Descriptor Set 切换
        cmd->PushConstants(ShaderType::Vertex, &command.transform, sizeof(PrismaMath::mat4));

        // 5. 提交绘制请求
        for (const auto& subMesh : command.mesh->GetSubMeshes()) {
            if (subMesh.VertexBuffer && subMesh.IndexBuffer) {
                cmd->SetVertexBuffer(subMesh.VertexBuffer.get(), 0);
                cmd->SetIndexBuffer(subMesh.IndexBuffer.get());
                cmd->DrawIndexed(subMesh.IndexCount);
            }
        }
    }
}

} // namespace Prisma::Graphic
