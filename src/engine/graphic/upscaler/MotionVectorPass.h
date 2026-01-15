#pragma once

#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IGBuffer.h"
#include "math/MathTypes.h"
#include <memory>

namespace PrismaEngine::Graphic {

// 前置声明
class ITexture;
class IShader;
class IBuffer;

/// @brief 运动矢量生成 Pass
/// 为超分辨率技术生成屏幕空间运动矢量
class MotionVectorPass : public IPass {
public:
    MotionVectorPass();
    ~MotionVectorPass() override;

    // === IPass 接口实现 ===

    const char* GetName() const override { return "MotionVectorPass"; }

    bool IsEnabled() const override { return m_enabled; }
    void SetEnabled(bool enabled) override { m_enabled = enabled; }

    void SetRenderTarget(IRenderTarget* renderTarget) override;
    void SetDepthStencil(IDepthStencil* depthStencil) override;
    void SetViewport(uint32_t width, uint32_t height) override;

    void Update(float deltaTime) override;
    void Execute(const PassExecutionContext& context) override;

    uint32_t GetPriority() const override { return m_priority; }
    void SetPriority(uint32_t priority) override { m_priority = priority; }

    // === 输入设置 ===

    /// @brief 设置当前帧深度
    void SetCurrentDepth(IDepthStencil* depth) { m_currentDepth = depth; }

    /// @brief 设置上一帧深度
    void SetPreviousDepth(IDepthStencil* depth) { m_previousDepth = depth; }

    /// @brief 设置 G-Buffer（用于世界空间位置重建）
    void SetGBuffer(IGBuffer* gBuffer) { m_gBuffer = gBuffer; }

    // === 输出 ===

    /// @brief 获取运动矢量渲染目标
    IRenderTarget* GetMotionVectorOutput() const { return m_motionVectorOutput; }

    // === 相机信息 ===

    /// @brief 更新相机信息
    void UpdateCameraInfo(const PrismaMath::mat4& view,
                          const PrismaMath::mat4& projection,
                          const PrismaMath::mat4& prevViewProjection);

private:
    bool InitializeResources();
    void ReleaseResources();
    bool CreateShaders();

    // 资源
    IRenderTarget* m_motionVectorOutput = nullptr;
    IDepthStencil* m_currentDepth = nullptr;
    IDepthStencil* m_previousDepth = nullptr;
    IGBuffer* m_gBuffer = nullptr;

    // 相机信息常量缓冲区
    struct CameraConstants {
        PrismaMath::mat4 inverseViewProjection;
        PrismaMath::mat4 previousViewProjection;
        PrismaMath::vec2 resolution;
        float padding0;
        float padding1;
    };
    IBuffer* m_cameraConstants = nullptr;

    // 相机信息
    PrismaMath::mat4 m_invView;
    PrismaMath::mat4 m_invProj;
    PrismaMath::mat4 m_prevViewProj;

    // 着色器
    IShader* m_motionVectorShader = nullptr;

    // 分辨率
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // Pass 状态
    bool m_enabled = true;
    uint32_t m_priority = 500;  // 在 GeometryPass 之后执行
};

} // namespace PrismaEngine::Graphic
