#pragma once

#include "Material.h"
#include "Mesh.h"
#include "interfaces/IPipeline.h"
#include "interfaces/RenderTypes.h"

namespace Prisma::Graphic {

/**
 * @brief 渲染指令 (Draw Command)
 * 极其轻量级，只存必要的变换和句柄。
 */
struct RenderCommand {
    Mesh* mesh;
    Material* material;
    PrismaMath::mat4 transform;
    BoundingBox boundingBox;
    Prisma::Color color;
};

/**
 * @brief 高层渲染器入口
 * 静态 API，方便应用层提交。
 */
class ENGINE_API Renderer {
public:
    struct SceneData {
        CameraData camera;
        std::vector<RenderCommand> commands;
    };

    // 渲染生命周期
    static void BeginScene(const CameraData& camera);
    static void EndScene();

    // 提交渲染指令
    static void Submit(Mesh* mesh, Material* material, const PrismaMath::mat4& transform, const Prisma::Color& color = Prisma::Color(1.0f, 1.0f, 1.0f, 1.0f));

    // 获取当前的待处理队列 (由 Pipeline 调用)
    static const std::vector<RenderCommand>& GetCommandQueue();

    // 获取当前场景数据
    static const SceneData& GetSceneData();

    // 清空队列
    static void ClearQueue();

private:
    static SceneData s_Data;
};

}  // namespace Prisma::Graphic
