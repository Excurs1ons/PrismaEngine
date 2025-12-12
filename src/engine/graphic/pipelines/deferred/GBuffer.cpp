#include "GBuffer.h"
#include "RenderCommandContext.h"
#include "Logger.h"

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Deferred {

// G-Buffer格式定义
const uint32_t GBufferFormats::POSITION_FORMAT = 10;  // DXGI_FORMAT_R16G16B16A16_FLOAT
const uint32_t GBufferFormats::NORMAL_FORMAT = 10;    // DXGI_FORMAT_R16G16B16A16_FLOAT
const uint32_t GBufferFormats::ALBEDO_FORMAT = 28;     // DXGI_FORMAT_R8G8B8A8_UNORM
const uint32_t GBufferFormats::EMISSIVE_FORMAT = 26;   // DXGI_FORMAT_R11G11B10_FLOAT
const uint32_t GBufferFormats::DEPTH_FORMAT = 40;      // DXGI_FORMAT_D32_FLOAT

GBuffer::GBuffer()
{
    LOG_DEBUG("GBuffer", "GBuffer构造函数被调用");
}

GBuffer::~GBuffer()
{
    LOG_DEBUG("GBuffer", "GBuffer析构函数被调用");
    Destroy();
}

bool GBuffer::Create(uint32_t width, uint32_t height)
{
    LOG_INFO("GBuffer", "创建G-Buffer: {0}x{1}", width, height);

    if (m_created) {
        LOG_WARNING("GBuffer", "G-Buffer已经创建，先销毁旧的");
        Destroy();
    }

    m_width = width;
    m_height = height;

    // TODO: 实现具体的渲染目标创建
    // 这里需要根据具体的图形API（DirectX12/Vulkan）来实现
    // 1. 创建位置+粗糙度纹理
    // 2. 创建法线+金属度纹理
    // 3. 创建颜色+AO纹理
    // 4. 创建自发光+材质ID纹理
    // 5. 创建深度缓冲
    // 6. 创建对应的渲染目标视图和着色器资源视图

    m_created = true;
    LOG_INFO("GBuffer", "G-Buffer创建成功");
    return true;
}

void GBuffer::Destroy()
{
    if (!m_created) {
        return;
    }

    LOG_DEBUG("GBuffer", "销毁G-Buffer");

    // TODO: 释放所有资源
    for (uint32_t i = 0; i < static_cast<uint32_t>(GBufferTarget::Count); ++i) {
        // 释放渲染目标资源
        if (m_renderTargets[i].resource) {
            // 释放资源
            m_renderTargets[i].resource = nullptr;
        }
        if (m_renderTargets[i].renderTargetView) {
            // 释放RTV
            m_renderTargets[i].renderTargetView = nullptr;
        }
        if (m_renderTargets[i].shaderResourceView) {
            // 释放SRV
            m_renderTargets[i].shaderResourceView = nullptr;
        }
    }

    // 释放深度缓冲
    if (m_depthBuffer) {
        m_depthBuffer = nullptr;
    }
    if (m_depthStencilView) {
        m_depthStencilView = nullptr;
    }
    if (m_depthShaderResourceView) {
        m_depthShaderResourceView = nullptr;
    }

    m_created = false;
    LOG_DEBUG("GBuffer", "G-Buffer销毁完成");
}

void GBuffer::Resize(uint32_t width, uint32_t height)
{
    if (m_width == width && m_height == height) {
        return;
    }

    LOG_DEBUG("GBuffer", "调整G-Buffer尺寸: {0}x{1} -> {2}x{3}",
              m_width, m_height, width, height);

    // 重新创建资源
    Create(width, height);
}

void GBuffer::SetAsRenderTarget(RenderCommandContext* context)
{
    if (!context || !m_created) {
        LOG_ERROR("GBuffer", "无效的上下文或G-Buffer未创建");
        return;
    }

    LOG_DEBUG("GBuffer", "设置G-Buffer为渲染目标");

    // TODO: 设置多个渲染目标
    void* renderTargets[4] = {
        GetRenderTargetView(GBufferTarget::Position),
        GetRenderTargetView(GBufferTarget::Normal),
        GetRenderTargetView(GBufferTarget::Albedo),
        GetRenderTargetView(GBufferTarget::Emissive)
    };

    // 设置MRT
    context->SetRenderTargets(renderTargets, 4, GetDepthStencilView());
}

void GBuffer::SetAsShaderResources(RenderCommandContext* context)
{
    if (!context || !m_created) {
        LOG_ERROR("GBuffer", "无效的上下文或G-Buffer未创建");
        return;
    }

    LOG_DEBUG("GBuffer", "设置G-Buffer为着色器资源");

    // TODO: 绑定所有G-Buffer纹理为着色器资源
    context->SetShaderResource("GBufferPosition", GetShaderResourceView(GBufferTarget::Position));
    context->SetShaderResource("GBufferNormal", GetShaderResourceView(GBufferTarget::Normal));
    context->SetShaderResource("GBufferAlbedo", GetShaderResourceView(GBufferTarget::Albedo));
    context->SetShaderResource("GBufferEmissive", GetShaderResourceView(GBufferTarget::Emissive));
    context->SetShaderResource("GBufferDepth", m_depthShaderResourceView);
}

void GBuffer::Clear(RenderCommandContext* context)
{
    if (!context || !m_created) {
        LOG_ERROR("GBuffer", "无效的上下文或G-Buffer未创建");
        return;
    }

    LOG_DEBUG("GBuffer", "清除G-Buffer");

    // TODO: 清除所有渲染目标
    // 位置缓冲清除为0
    // 法线缓冲清除为(0, 0, 1)
    // 颜色缓冲清除为0
    // 自发光缓冲清除为0
    // 深度缓冲清除为1.0
}

void* GBuffer::GetRenderTargetView(GBufferTarget target) const
{
    uint32_t index = static_cast<uint32_t>(target);
    if (index >= static_cast<uint32_t>(GBufferTarget::Count)) {
        return nullptr;
    }
    return m_renderTargets[index].renderTargetView;
}

void* GBuffer::GetShaderResourceView(GBufferTarget target) const
{
    uint32_t index = static_cast<uint32_t>(target);
    if (index >= static_cast<uint32_t>(GBufferTarget::Count)) {
        return nullptr;
    }
    return m_renderTargets[index].shaderResourceView;
}

void* GBuffer::GetDepthStencilView() const
{
    return m_depthStencilView;
}

} // namespace Deferred
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine