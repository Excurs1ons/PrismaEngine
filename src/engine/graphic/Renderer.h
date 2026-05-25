#pragma once

#include "Material.h"
#include "Mesh.h"
#include "interfaces/IPipeline.h"
#include "interfaces/RenderTypes.h"

namespace Prisma::Graphic {

/* 渲染指令 (Draw Command) */
struct RenderCommand {
    Mesh* mesh;
    Material* material;
    PrismaMath::mat4 transform;
    BoundingBox boundingBox;
    Prisma::Color color;
};

/* 高层渲染器入口 */
class ENGINE_API Renderer {
public:
    struct SceneData {
        CameraData camera;
        std::vector<RenderCommand> commands;
        std::vector<RenderCommand> gizmoCommands;
    };

    // 渲染生命周期
    static void BeginScene(const CameraData& camera);
    static void EndScene();

    // 提交渲染指令
    static void Submit(Mesh* mesh, Material* material, const PrismaMath::mat4& transform, const Prisma::Color& color = Prisma::Color(1.0f, 1.0f, 1.0f, 1.0f));

    // 提交 Gizmo 渲染指令（不受光照影响）
    static void SubmitGizmo(Mesh* mesh, Material* material, const PrismaMath::mat4& transform, const Prisma::Color& color = Prisma::Color(1.0f, 1.0f, 1.0f, 1.0f));

    // 获取当前的待处理队列 (由 Pipeline 调用)
    static const std::vector<RenderCommand>& GetCommandQueue();

    // 获取 Gizmo 队列
    static const std::vector<RenderCommand>& GetGizmoQueue();

    // 获取当前场景数据
    static const SceneData& GetSceneData();

    // 清空队列
    static void ClearQueue();
    static void ClearGizmoQueue();

private:
    static SceneData s_Data;
};

}  // namespace Prisma::Graphic
