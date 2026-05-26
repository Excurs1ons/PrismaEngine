#pragma once

#include "graphic/LogicalPass.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/ISampler.h"
#include <memory>

namespace Prisma::Graphic {

/**
 * @brief PixelPerfectPass — 256×224 离屏渲染 + 整数倍最近邻上采样
 *
 * 创建一个固定分辨率的离屏 RenderTarget，将 2D 主内容（Opaque + Canvas）
 * 渲染到该目标上，然后以整数倍最近邻采样上采样到交换链。
 * 适用于 Retro / Pixel-Art 风格的 2D 游戏。
 */
class PixelPerfectPass : public LogicalPass {
public:
    PixelPerfectPass();
    ~PixelPerfectPass() override = default;

    // IPass 接口
    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override { UpdateTime(ts); }

    void SetLogicalWidth(uint32_t w) { m_logicW = w; }
    void SetLogicalHeight(uint32_t h) { m_logicH = h; }
    uint32_t GetLogicalWidth() const { return m_logicW; }
    uint32_t GetLogicalHeight() const { return m_logicH; }

    /// @brief 获取离屏纹理（用于 BeginRenderPass）
    ITexture* GetOffscreenTexture() const { return m_offscreenTexture.get(); }

    /// @brief 获取离屏渲染目标
    IRenderTarget* GetOffscreenTarget() const { return m_offscreenRT.get(); }

    /// @brief 获取深度纹理（用于离屏 render pass 的 depthStencil）
    ITexture* GetDepthTexture() const { return m_depthTexture.get(); }

    /// @brief 获取点采样器（最近邻采样，用于 blit）
    ISampler* GetPointSampler() const { return m_pointSampler.get(); }

    void Initialize(IRenderDevice* device);
    void Shutdown();

    /**
     * @brief 将离屏 RT 上采样到交换链
     * @param cmd            命令缓冲区
     * @param device         渲染设备
     * @param swapChainTarget 交换链目标（用于获取尺寸，可为nullptr）
     */
    void BlitToSwapChain(ICommandBuffer* cmd, IRenderDevice* device, IRenderTarget* swapChainTarget);

    /**
     * @brief 设置清除颜色（离屏 RT 的 clear color）
     */
    void SetClearColor(float r, float g, float b, float a) {
        m_clearR = r; m_clearG = g; m_clearB = b; m_clearA = a;
    }

    /**
     * @brief 获取清除颜色数组（RGBA，每分量 0-1）
     */
    const float* GetClearColorArray() const { return &m_clearR; }

    /**
     * @brief 获取清除颜色
     */
    Color GetClearColor() const { return Color(m_clearR, m_clearG, m_clearB, m_clearA); }

    bool IsPixelPerfectEnabled() const { return m_enabled; }
    void SetPixelPerfectEnabled(bool enabled) { m_enabled = enabled; }

private:
    void EnsureResources(IRenderDevice* device);

    /** 计算整数缩放后的视口位置与大小 */
    void CalculateViewport(uint32_t windowW, uint32_t windowH,
                           int& outX, int& outY,
                           uint32_t& outW, uint32_t& outH, uint32_t& scale);

    /// @brief 离屏 RT 代理（遵循 LightTextureRTProxy 模式）
    class OffscreenRTProxy final : public ITextureRenderTarget {
    public:
        explicit OffscreenRTProxy(std::shared_ptr<ITexture> tex) : m_tex(std::move(tex)) {}

        uint32_t GetWidth() const override {
            return m_tex ? static_cast<uint32_t>(m_tex->GetWidth()) : 0;
        }
        uint32_t GetHeight() const override {
            return m_tex ? static_cast<uint32_t>(m_tex->GetHeight()) : 0;
        }
        TextureFormat GetFormat() const override {
            return m_tex ? m_tex->GetFormat() : TextureFormat::Unknown;
        }
        TextureType GetType() const override {
            return m_tex ? m_tex->GetTextureType() : TextureType::Texture2D;
        }
        void* GetNativeHandle() const override { return nullptr; }
        bool IsSwapChain() const override { return false; }
        void Clear(const float color[4]) override {
            if (m_tex) {
                m_tex->Clear(Color(color[0], color[1], color[2], color[3]));
            }
        }
        uint32_t GetMipLevels() const override { return m_tex ? m_tex->GetMipLevels() : 1; }
        uint32_t GetArraySize() const override { return m_tex ? m_tex->GetArraySize() : 1; }
        ITexture* GetTexture() override { return m_tex.get(); }

    private:
        std::shared_ptr<ITexture> m_tex;
    };

    uint32_t m_logicW = 256;
    uint32_t m_logicH = 224;
    bool m_enabled = true;

    // 清除颜色
    float m_clearR = 0.0f, m_clearG = 0.0f, m_clearB = 0.0f, m_clearA = 1.0f;

    // 离屏资源
    std::shared_ptr<ITexture> m_offscreenTexture;
    std::shared_ptr<ITextureRenderTarget> m_offscreenRT;
    std::shared_ptr<ITexture> m_depthTexture;

    // Blit 管线
    std::shared_ptr<IPipelineState> m_blitPSO;
    std::shared_ptr<IShader> m_vertShader;
    std::shared_ptr<IShader> m_fragShader;
    std::shared_ptr<ISampler> m_pointSampler;

    // 描述符集用于 blit 时绑定离屏纹理
    std::shared_ptr<IDescriptorSetLayout> m_blitDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_blitDescSet;
};

} // namespace Prisma::Graphic
