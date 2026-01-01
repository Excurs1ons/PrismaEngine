#pragma once

#include "IRenderTarget.h"
#include "RenderTypes.h"
#include "math/MathTypes.h"
#include <string>
#include "interfaces/IDeviceContext.h"

namespace PrismaEngine::Graphic {

/// @brief 场景数据结构
/// 包含 Pass 执行所需的场景信息
struct SceneData {
    // 相机数据
    struct {
        PrismaMath::mat4 view;
        PrismaMath::mat4 projection;
        PrismaMath::mat4 viewProjection;
        PrismaMath::vec3 position;
        PrismaMath::vec3 direction;
        float nearPlane;
        float farPlane;
    } camera;

    // 时间数据
    struct {
        float deltaTime;
        float totalTime;
    } time;

    // 屏幕尺寸
    struct {
        uint32_t width;
        uint32_t height;
    } viewport;

    // 全局光照数据
    struct {
        PrismaMath::vec3 ambientColor;
        float ambientIntensity;
    } lighting;

    SceneData() {
        camera.view = PrismaMath::mat4(1.0f);
        camera.projection = PrismaMath::mat4(1.0f);
        camera.viewProjection = PrismaMath::mat4(1.0f);
        camera.position = PrismaMath::vec3(0.0f, 0.0f, 0.0f);
        camera.direction = PrismaMath::vec3(0.0f, 0.0f, -1.0f);
        camera.nearPlane = 0.1f;
        camera.farPlane = 1000.0f;

        time.deltaTime = 0.0f;
        time.totalTime = 0.0f;

        viewport.width = 1920;
        viewport.height = 1080;

        lighting.ambientColor = PrismaMath::vec3(0.1f, 0.1f, 0.1f);
        lighting.ambientIntensity = 1.0f;
    }
};

/// @brief Pass 执行描述符
/// 包含 Pass 执行时所需的所有资源
struct PassExecutionContext {
    IDeviceContext* deviceContext;      // 设备上下文
    IRenderTarget* renderTarget;         // 主渲染目标
    IDepthStencil* depthStencil;         // 深度模板缓冲区
    const SceneData* sceneData;          // 场景数据

    PassExecutionContext()
        : deviceContext(nullptr)
        , renderTarget(nullptr)
        , depthStencil(nullptr)
        , sceneData(nullptr) {}
};

/// @brief 逻辑 Pass 抽象接口
/// 职责：定义渲染内容，不包含具体图形 API
/// Pipeline 负责管理和执行 IPass
class IPass {
public:
    virtual ~IPass() = default;

    /// @brief 获取 Pass 名称
    virtual const char* GetName() const = 0;

    /// @brief Pass 是否启用
    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool enabled) = 0;

    /// @brief 设置渲染目标
    /// @param renderTarget 渲染目标接口指针
    virtual void SetRenderTarget(IRenderTarget* renderTarget) = 0;

    /// @brief 设置深度模板
    /// @param depthStencil 深度模板接口指针
    virtual void SetDepthStencil(IDepthStencil* depthStencil) = 0;

    /// @brief 设置视口大小
    /// @param width 宽度
    /// @param height 高度
    virtual void SetViewport(uint32_t width, uint32_t height) = 0;

    /// @brief 更新 Pass 数据
    /// @param deltaTime 时间增量
    virtual void Update(float deltaTime) = 0;

    /// @brief 执行 Pass
    /// @param context 执行上下文
    virtual void Execute(const PassExecutionContext& context) = 0;

    /// @brief 获取 Pass 执行优先级（数值越小越先执行）
    virtual uint32_t GetPriority() const = 0;

    /// @brief 设置 Pass 执行优先级
    virtual void SetPriority(uint32_t priority) = 0;
};

/// @brief 逻辑 Pipeline 抽象接口
/// 职责：管理和执行 IPass
class ILogicalPipeline {
public:
    virtual ~ILogicalPipeline() = default;

    /// @brief 获取 Pipeline 名称
    virtual const char* GetName() const = 0;

    /// @brief 添加 Pass
    /// @param pass Pass 接口指针
    /// @return 是否添加成功
    virtual bool AddPass(IPass* pass) = 0;

    /// @brief 移除 Pass
    /// @param pass Pass 接口指针
    /// @return 是否移除成功
    virtual bool RemovePass(IPass* pass) = 0;

    /// @brief 获取 Pass 数量
    virtual size_t GetPassCount() const = 0;

    /// @brief 获取 Pass
    /// @param index 索引
    /// @return Pass 接口指针
    virtual IPass* GetPass(size_t index) const = 0;

    /// @brief 按名称查找 Pass
    /// @param name Pass 名称
    /// @return Pass 接口指针，未找到返回 nullptr
    virtual IPass* FindPass(const char* name) const = 0;

    /// @brief 执行 Pipeline
    /// @param context 执行上下文
    virtual void Execute(const PassExecutionContext& context) = 0;

    /// @brief 设置视口大小（通知所有 Pass）
    /// @param width 宽度
    /// @param height 高度
    virtual void SetViewport(uint32_t width, uint32_t height) = 0;

    /// @brief 设置主渲染目标
    /// @param renderTarget 渲染目标接口指针
    virtual void SetRenderTarget(IRenderTarget* renderTarget) = 0;

    /// @brief 设置深度模板
    /// @param depthStencil 深度模板接口指针
    virtual void SetDepthStencil(IDepthStencil* depthStencil) = 0;
};

} // namespace PrismaEngine::Graphic
