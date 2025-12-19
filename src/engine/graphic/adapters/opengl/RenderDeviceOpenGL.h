#pragma once

#include "../interfaces/IRenderDevice.h"
#include "../interfaces/ICommandBuffer.h"
#include "../interfaces/IFence.h"
#include "../interfaces/ISwapChain.h"
#include "../interfaces/IResourceFactory.h"

// OpenGL headers
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace PrismaEngine::Graphic::OpenGL {

// 前置声明
class OpenGLCommandBuffer;
class OpenGLFence;
class OpenGLSwapChain;
class OpenGLResourceFactory;

/// @brief OpenGL渲染设备
/// 实现IRenderDevice接口，基于OpenGL 4.6核心配置
class RenderDeviceOpenGL : public IRenderDevice {
public:
    /// @brief 构造函数
    RenderDeviceOpenGL();

    /// @brief 析构函数
    ~RenderDeviceOpenGL() override;

    // ========== IRenderDevice接口实现 ==========

    /// @brief 初始化设备
    bool Initialize(const DeviceDesc& desc) override;

    /// @brief 关闭设备
    void Shutdown() override;

    /// @brief 获取设备名称
    std::string GetName() const override;

    /// @brief 获取API名称
    std::string GetAPIName() const override;

    // ========== 命令缓冲区管理 ==========

    /// @brief 创建命令缓冲区
    std::unique_ptr<ICommandBuffer> CreateCommandBuffer(CommandBufferType type) override;

    /// @brief 提交命令缓冲区
    void SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence = nullptr) override;

    /// @brief 批量提交命令缓冲区
    void SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers,
                             const std::vector<IFence*>& fences = {}) override;

    // ========== 同步操作 ==========

    /// @brief 等待设备空闲
    void WaitForIdle() override;

    /// @brief 创建围栏
    std::unique_ptr<IFence> CreateFence() override;

    /// @brief 等待围栏
    void WaitForFence(IFence* fence) override;

    // ========== 资源管理 ==========

    /// @brief 获取资源工厂
    IResourceFactory* GetResourceFactory() const override;

    // ========== 交换链管理 ==========

    /// @brief 创建交换链
    std::unique_ptr<ISwapChain> CreateSwapChain(void* windowHandle,
                                               uint32_t width,
                                               uint32_t height,
                                               bool vsync = true) override;

    /// @brief 获取当前交换链
    ISwapChain* GetSwapChain() const override;

    // ========== 帧管理 ==========

    /// @brief 开始帧
    void BeginFrame() override;

    /// @brief 结束帧
    void EndFrame() override;

    /// @brief 呈现
    void Present() override;

    // ========== 功能查询 ==========

    /// @brief 是否支持多线程
    bool SupportsMultiThreaded() const override;

    /// @brief 是否支持无绑定纹理
    bool SupportsBindlessTextures() const override;

    /// @brief 是否支持计算着色器
    bool SupportsComputeShader() const override;

    /// @brief 是否支持光线追踪
    bool SupportsRayTracing() const override;

    /// @brief 是否支持网格着色器
    bool SupportsMeshShader() const override;

    /// @brief 是否支持可变速率着色
    bool SupportsVariableRateShading() const override;

    // ========== 渲染统计 ==========

    /// @brief 获取GPU内存信息
    GPUMemoryInfo GetGPUMemoryInfo() const override;

    /// @brief 获取渲染统计
    RenderStats GetRenderStats() const override;

    // ========== 调试功能 ==========

    /// @brief 开始调试标记
    void BeginDebugMarker(const std::string& name) override;

    /// @brief 结束调试标记
    void EndDebugMarker() override;

    /// @brief 设置调试标记
    void SetDebugMarker(const std::string& name) override;

    // ========== OpenGL特定方法 ==========

    /// @brief 获取OpenGL上下文
    /// @return GLFW窗口指针
    GLFWwindow* GetGLFWWindow() const;

    /// @brief 获取主帧缓冲
    /// @return 帧缓冲ID
    GLuint GetDefaultFBO() const;

    /// @brief 绑定默认帧缓冲
    void BindDefaultFramebuffer();

    /// @brief 检查OpenGL扩展
    /// @param extension 扩展名
    /// @return 是否支持
    bool IsExtensionSupported(const std::string& extension) const;

    /// @brief 获取OpenGL版本
    /// @return 版本字符串
    std::string GetOpenGLVersion() const;

    /// @brief 获取着色器语言版本
    /// @return GLSL版本字符串
    std::string GetGLSLVersion() const;

private:
    // ========== 初始化相关 ==========

    /// @brief 创建GLFW窗口
    bool CreateGLFWWindow(const DeviceDesc& desc);

    /// @brief 初始化OpenGL上下文
    bool InitializeOpenGL();

    /// @brief 加载OpenGL函数
    bool LoadOpenGLFunctions();

    /// @brief 初始化调试输出
    void InitializeDebugOutput();

    /// @brief 查询设备能力
    void QueryDeviceCapabilities();

    /// @brief 释放所有资源
    void ReleaseAll();

    // ========== 错误检查 ==========

    /// @brief 检查OpenGL错误
    bool CheckOpenGLError(const std::string& operation);

    /// @brief OpenGL调试回调
    static void DebugCallback(GLenum source, GLenum type, GLuint id,
                              GLenum severity, GLsizei length,
                              const GLchar* message, const void* userParam);

    // ========== 扩展管理 ==========

    /// @brief 加载扩展
    void LoadExtensions();

    // ========== 成员变量 ==========

    // GLFW窗口和上下文
    GLFWwindow* m_window = nullptr;
    std::string m_title = "Prisma Engine OpenGL";

    // 交换链
    std::unique_ptr<OpenGLSwapChain> m_swapChain;

    // 资源工厂
    std::unique_ptr<OpenGLResourceFactory> m_resourceFactory;

    // 设备描述
    DeviceDesc m_desc;

    // 设备能力
    struct DeviceCapabilities {
        bool multiThreaded = false;
        bool bindlessTextures = false;
        bool computeShaders = false;
        bool rayTracing = false;
        bool meshShaders = false;
        bool variableRateShading = false;
        bool directStateAccess = false;
        bool textureView = false;
        bool shaderStorage = false;
        int maxTextureSize = 0;
        int maxSamples = 0;
        int maxUniformBufferSize = 0;
        int maxShaderStorageBufferSize = 0;
        int maxComputeWorkGroupInvocations = 0;
    } m_capabilities;

    // 渲染统计
    RenderStats m_stats;

    // 默认帧缓冲
    GLuint m_defaultFBO = 0;

    // 支持的扩展列表
    std::unordered_map<std::string, bool> m_extensions;

    // 调试相关
    bool m_debugEnabled = false;
    std::vector<std::string> m_debugMarkers;

    // 当前帧索引
    uint32_t m_frameIndex = 0;

    // 同步对象
    std::vector<std::unique_ptr<OpenGLFence>> m_frameFences;
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

    // 状态
    bool m_initialized = false;
};

} // namespace PrismaEngine::Graphic::OpenGL