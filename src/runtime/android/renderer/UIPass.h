#pragma once

#include "RenderPass.h"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>
namespace PrismaEngine {

class UIComponent;

// UI 顶点结构（与 ui.vert 着色器匹配）
struct UIVertex {
    float position[2];  // vec2 inPosition
    float uv[2];        // vec2 inUV
    float color[4];     // vec4 inColor
};

/// @brief UI 渲染 Pass（Android runtime 适配器）
///
/// 功能：
/// - 渲染 2D UI 组件（按钮、文本等）
/// - 使用屏幕坐标系（左上角为原点）
/// - 支持透明混合
///
/// 特点：
/// - 无深度测试（UI 总是在最上层）
/// - Alpha 混合（src_alpha, one_minus_src_alpha）
/// - 双面渲染（无背面剔除）
///
/// API 切换注意事项：
/// - pipeline_ 是 Vulkan 特有的 VkPipeline
/// - DirectX 12 使用 PSO (Pipeline State Object)
/// - Metal 使用 MTLRenderPipelineState
class UIPass : public RenderPass {
public:
    UIPass(): RenderPass("UI Pass") {}
    ~UIPass() override = default;

    // 禁止拷贝
    UIPass(const UIPass&) = delete;
    UIPass& operator=(const UIPass&) = delete;

    // 允许移动
    UIPass(UIPass&&) = default;
    UIPass& operator=(UIPass&&) = default;

    // === RenderPass 接口实现 ===

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    // === 配置方法 ===

    /// @brief 添加 UI 组件到渲染列表
    void addUIComponent(std::shared_ptr<UIComponent> component);

    /// @brief 设置 SwapChain 扩展信息（用于 viewport/scissor）
    void setSwapChainExtent(VkExtent2D extent);

    /// @brief 设置 android_app（用于加载着色器）
    void setAndroidApp(android_app* app);

    /// @brief 设置物理设备（用于查找内存类型）
    void setPhysicalDevice(VkPhysicalDevice physicalDevice);

    /// @brief 设置当前帧索引
    void setCurrentFrame(uint32_t currentFrame);

    /// @brief 设置内容区域偏移（状态栏高度等）
    void setContentOffset(int32_t offsetX, int32_t offsetY);

private:
    /**
     * 创建 UI 渲染管线
     * 使用 ui.vert 和 ui.frag 着色器
     */
    void createPipeline(VkDevice device, VkRenderPass renderPass);

    /**
     * 更新 UI 顶点缓冲区
     * 将所有 UI 组件转换为顶点数据
     */
    void updateVertexBuffer(VkDevice device);

    /**
     * 查找内存类型（内部辅助方法）
     */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    /**
     * 创建顶点缓冲区
     */
    void createVertexBuffer(VkDevice device);

    // === 数据成员 ===

    std::vector<std::shared_ptr<UIComponent>> m_uiComponents;

    // [Vulkan-Specific] 以下成员变量在切换 API 时需要修改或移除
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;

    VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;

    // 配置信息
    VkExtent2D swapChainExtent_{};  // SwapChain 扩展
    android_app* app_ = nullptr;  // android_app（用于加载着色器）
    uint32_t currentFrame_ = 0;  // 当前帧索引
    VkDevice device_ = VK_NULL_HANDLE;  // Vulkan 设备（用于更新顶点缓冲区）
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;  // 物理设备（用于查找内存类型）

    // 内容区域偏移（状态栏、导航栏等）
    int32_t contentOffsetX_ = 0;
    int32_t contentOffsetY_ = 0;

    // 顶点数据缓存
    std::vector<UIVertex> m_vertexData;
    bool m_vertexDataDirty = true;  // 顶点数据是否需要更新
};

} // namespace PrismaEngine
