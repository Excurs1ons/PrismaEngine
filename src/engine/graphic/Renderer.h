#pragma once

#include "Mesh.h"
#include "Material.h"
#include "interfaces/RenderTypes.h"
#include "interfaces/IPipeline.h"

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
};

/**
 * @brief 高层渲染器入口
 * 静态 API，方便应用层提交。
 */
class ENGINE_API Renderer {
public:
    // 渲染生命周期
    static void BeginScene(const CameraData& camera);
    static void EndScene();

    // 提交渲染指令
    static void Submit(Mesh* mesh, Material* material, const PrismaMath::mat4& transform);

    // 获取当前的待处理队列 (由 Pipeline 调用)
    static const std::vector<RenderCommand>& GetCommandQueue();

    // 清空队列
    static void ClearQueue();

private:
    struct SceneData {
        CameraData camera;
        std::vector<RenderCommand> commands;
    };

    static SceneData s_Data;
};

} // namespace Prisma::Graphic
