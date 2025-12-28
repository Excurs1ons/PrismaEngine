#pragma once

#include "math/MathTypes.h"
#include <memory>

namespace PrismaEngine::Graphic {

// 前置声明
class RenderCommandContext;

// G-Buffer渲染目标结构
// 使用MRT（Multiple Render Targets）技术
enum class GBufferTarget : uint32_t {
    // 0: Position (RGB) + Roughness (A)
    Position = 0,
    // 1: Normal (RGB) + Metallic (A)
    Normal = 1,
    // 2: Albedo (RGB) + Ambient Occlusion (A)
    Albedo = 2,
    // 3: Emissive (RGB) + Material ID (A)
    Emissive = 3,
    // 深度缓冲（单独的深度纹理）
    Depth = 4,

    Count
};

// G- Buffer纹理格式定义
struct GBufferFormats {
    static const uint32_t POSITION_FORMAT;      // DXGI_FORMAT_R16G16B16A16_FLOAT
    static const uint32_t NORMAL_FORMAT;        // DXGI_FORMAT_R16G16B16A16_FLOAT
    static const uint32_t ALBEDO_FORMAT;        // DXGI_FORMAT_R8G8B8A8_UNORM
    static const uint32_t EMISSIVE_FORMAT;      // DXGI_FORMAT_R11G11B10_FLOAT
    static const uint32_t DEPTH_FORMAT;         // DXGI_FORMAT_D32_FLOAT
};

// G-Buffer数据结构（用于着色器）
struct GBufferData {
    // World space position
    PrismaMath::vec3 position;
    float padding1;

    // World space normal
    PrismaMath::vec3 normal;
    float roughness;

    // Albedo color
    PrismaMath::vec3 albedo;
    float metallic;

    // Emissive color
    PrismaMath::vec3 emissive;
    float ao;

    // Material properties
    uint32_t materialID;
    float padding2[3];
};

// G-Buffer资源管理器
class GBuffer
{
public:
    GBuffer();
    ~GBuffer();

    // 创建G-Buffer资源
    bool Create(uint32_t width, uint32_t height);

    // 销毁G-Buffer资源
    void Destroy();

    // 调整G-Buffer尺寸
    void Resize(uint32_t width, uint32_t height);

    // 设置为渲染目标
    void SetAsRenderTarget(RenderCommandContext* context);

    // 设置为着色器资源
    void SetAsShaderResources(RenderCommandContext* context);

    // 清除G-Buffer
    void Clear(RenderCommandContext* context);

    // 获取渲染目标视图
    void* GetRenderTargetView(GBufferTarget target) const;

    // 获取着色器资源视图
    void* GetShaderResourceView(GBufferTarget target) const;

    // 获取深度缓冲区视图
    void* GetDepthStencilView() const;

    // 获取宽度
    uint32_t GetWidth() const { return m_width; }

    // 获取高度
    uint32_t GetHeight() const { return m_height; }

private:
    // 渲染目标资源
    struct RenderTarget {
        void* resource = nullptr;
        void* renderTargetView = nullptr;
        void* shaderResourceView = nullptr;
    };

    RenderTarget m_renderTargets[static_cast<uint32_t>(GBufferTarget::Count)];

    // 深度缓冲
    void* m_depthBuffer = nullptr;
    void* m_depthStencilView = nullptr;
    void* m_depthShaderResourceView = nullptr;

    // 尺寸
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // 是否已创建
    bool m_created = false;
};

} // namespace PrismaEngine::Graphic