#pragma once

/// @file GeometryRenderPass.h
/// @deprecated 此文件已被弃用，请使用 graphic/pipelines/deferred/GeometryPass.h
/// @note 旧版 GeometryRenderPass 类已被新的 GeometryPass 替代
/// @note 此文件仅为向后兼容保留，将在未来版本中移除

#include "RenderPass.h"
#include <vector>
#include <memory>

// 前向声明
class Mesh;

namespace Engine {

/// @deprecated 使用 graphic/pipelines/deferred/GeometryPass.h 中的 GeometryPass 替代
class [[deprecated("Use GeometryPass from graphic/pipelines/deferred/GeometryPass.h instead")]] GeometryRenderPass : public RenderPass
{
public:
    GeometryRenderPass();
    ~GeometryRenderPass();

    void Execute(RenderCommandContext* context) override;
    void SetRenderTarget(void* renderTarget) override;
    void ClearRenderTarget(float r, float g, float b, float a) override;
    void SetViewport(uint32_t width, uint32_t height) override;
    void AddMeshToRenderQueue(std::shared_ptr<Mesh> mesh, const float* transform);

private:
    void* m_renderTarget = nullptr;
    float m_clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    struct RenderItem {
        std::shared_ptr<Mesh> mesh;
        float transform[16];
    };

    std::vector<RenderItem> m_renderQueue;
};

} // namespace Engine
