#pragma once

#include "graphic/RenderSystemNew.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IShader.h"
#include <memory>
#include <vector>

namespace PrismaEngine::Graphic {

/// @brief 新渲染系统的测试程序
/// 用于验证抽象接口和DX12适配器是否正常工作
class RenderSystemTest {
public:
    RenderSystemTest();
    ~RenderSystemTest() = default;

    /// @brief 初始化测试环境
    /// @param windowHandle 窗口句柄
    /// @param width 窗口宽度
    /// @param height 窗口高度
    /// @return 是否初始化成功
    bool Initialize(void* windowHandle, uint32_t width, uint32_t height);

    /// @brief 清理测试环境
    void Shutdown();

    /// @brief 运行测试
    /// @return 是否所有测试通过
    bool RunTests();

    /// @brief 渲染一帧
    void RenderFrame();

private:
    // === 测试方法 ===

    /// @brief 测试设备初始化
    bool TestDeviceInitialization();

    /// @brief 测试资源管理器
    bool TestResourceManager();

    /// @brief 测试着色器编译
    bool TestShaderCompilation();

    /// @brief 测试缓冲区创建
    bool TestBufferCreation();

    /// @brief 测试纹理创建
    bool TestTextureCreation();

    /// @brief 测试管线状态对象
    bool TestPipelineState();

    /// @brief 测试渲染流程
    bool TestRenderPipeline();

    /// @brief 测试资源清理
    bool TestResourceCleanup();

    // === 辅助方法 ===

    /// @brief 创建简单三角形着色器
    bool CreateTriangleShaders();

    /// @brief 创建三角形几何体
    bool CreateTriangleGeometry();

    /// @brief 创建渲染管线
    bool CreateRenderPipeline();

    // === 成员变量 ===

    std::unique_ptr<RenderSystem> m_renderSystem;
    IRenderDevice* m_device = nullptr;
    IResourceManager* m_resourceManager = nullptr;

    // 测试资源
    std::shared_ptr<IShader> m_vertexShader;
    std::shared_ptr<IShader> m_pixelShader;
    std::shared_ptr<IBuffer> m_vertexBuffer;
    std::shared_ptr<IPipelineState> m_pipelineState;

    // 窗口参数
    void* m_windowHandle = nullptr;
    uint32_t m_width = 800;
    uint32_t m_height = 600;

    // 测试结果
    bool m_initialized = false;
    std::vector<std::string> m_testResults;
};

} // namespace PrismaEngine::Graphic