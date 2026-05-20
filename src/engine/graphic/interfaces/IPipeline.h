#pragma once

#include "RenderTypes.h"
#include <memory>
#include <string>
#include <vector>

namespace Prisma { class Scene; }

namespace Prisma::Graphic {

class IRenderDevice;
class ICommandBuffer;
class RenderPass;

/**
 * @brief 强类型相机数据
 */
struct CameraData {
    PrismaMath::mat4 viewMatrix;
    PrismaMath::mat4 projectionMatrix;
    PrismaMath::vec3 position;
    float nearPlane;
    float farPlane;
    float fov = 70.0f;
};

class ITexture;

/**
 * @brief 强类型渲染上下文
 */
struct RenderContext {
    IRenderDevice* device = nullptr;
    ICommandBuffer* commandBuffer = nullptr;
    
    // [改动] 目标纹理
    // 目的：支持离屏渲染（如编辑器视口）。若为 nullptr，则默认渲染到交换链。
    ITexture* targetTexture = nullptr;
    
    // 基础管线必须显式包含这些
    CameraData camera;
    std::vector<Light> lights;
    
    uint32_t frameIndex = 0;
    float deltaTime = 0.0f;
    uint32_t width = 0;
    uint32_t height = 0;
};

/**
 * @brief 渲染管线接口
 */
class IPipeline {
public:
    virtual ~IPipeline() = default;

    virtual int Initialize(IRenderDevice* device) = 0;
    virtual void Shutdown() = 0;

    // 执行渲染逻辑
    virtual void Execute(const RenderContext& ctx) = 0;

    // 场景加载后自动回调（引擎层在加载入口场景后调用）
    virtual void OnSceneLoaded(Scene* scene) {}

    // 基础管线不再需要 AddRenderPass 这种灵活得过头的接口，
    // 具体的管线（如 ForwardPipeline）应该内部固定好自己的 Pass。
};

} // namespace Prisma::Graphic
