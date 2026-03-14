#include "Renderer.h"
#include <algorithm>

namespace Prisma::Graphic {

// 静态数据初始化
Renderer::SceneData Renderer::s_Data;

void Renderer::BeginScene(const CameraData& camera) {
    s_Data.camera = camera;
    s_Data.commands.clear();
}

void Renderer::EndScene() {
    // 在这里可以进行一些全局的预处理，比如初步的视锥剔除 (Frustum Culling)
    // 或者根据材质对指令进行预排序，减少后续 Pass 的负担。
    
    // 简单的按材质排序，减少状态切换开销
    std::sort(s_Data.commands.begin(), s_Data.commands.end(), [](const RenderCommand& a, const RenderCommand& b) {
        return a.material < b.material;
    });
}

void Renderer::Submit(Mesh* mesh, Material* material, const PrismaMath::mat4& transform) {
    if (!mesh || !material) return;
    
    RenderCommand command;
    command.mesh = mesh;
    command.material = material;
    command.transform = transform;
    command.boundingBox = mesh->GetBoundingBox(); // 这里的包围盒以后可以预先变换
    
    s_Data.commands.push_back(command);
}

const std::vector<RenderCommand>& Renderer::GetCommandQueue() {
    return s_Data.commands;
}

void Renderer::ClearQueue() {
    s_Data.commands.clear();
}

} // namespace Prisma::Graphic
