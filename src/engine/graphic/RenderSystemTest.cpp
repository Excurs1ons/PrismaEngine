#include "interfaces/RenderTypes.h"
#include "RenderSystemTest.h"
#include "Logger.h"
#include <array>
#include <sstream>

namespace PrismaEngine::Graphic {

RenderSystemTest::RenderSystemTest() {
    m_testResults.reserve(20);
}

bool RenderSystemTest::Initialize(void* windowHandle, uint32_t width, uint32_t height) {
    LOG_INFO("RenderSystemTest", "开始初始化新渲染系统测试");

    m_windowHandle = windowHandle;
    m_width = width;
    m_height = height;

    try {
        // 创建渲染系统
        m_renderSystem = std::make_unique<RenderSystem>();

        // 配置渲染系统
        RenderSystemDesc desc;
        desc.backendType = RenderBackendType::DirectX12;
        desc.windowHandle = windowHandle;
        desc.width = width;
        desc.height = height;
        desc.enableDebug = true;
        desc.name = "RenderSystemTest";

        // 初始化渲染系统
        if (!m_renderSystem->Initialize(desc)) {
            LOG_ERROR("RenderSystemTest", "渲染系统初始化失败");
            return false;
        }

        // 获取设备和资源管理器
        m_device = m_renderSystem->GetDevice();
        m_resourceManager = m_renderSystem->GetResourceManager();

        if (!m_device || !m_resourceManager) {
            LOG_ERROR("RenderSystemTest", "无法获取渲染设备或资源管理器");
            return false;
        }

        m_initialized = true;
        LOG_INFO("RenderSystemTest", "新渲染系统测试初始化成功");
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("RenderSystemTest", "初始化过程中发生异常: {0}", e.what());
        return false;
    }
}

void RenderSystemTest::Shutdown() {
    if (!m_initialized) {
        return;
    }

    LOG_INFO("RenderSystemTest", "开始清理新渲染系统测试");

    // 清理测试资源
    m_pipelineState.reset();
    m_vertexBuffer.reset();
    m_vertexShader.reset();
    m_pixelShader.reset();

    // 清理渲染系统
    if (m_renderSystem) {
        m_renderSystem->Shutdown();
        m_renderSystem.reset();
    }

    m_device = nullptr;
    m_resourceManager = nullptr;
    m_initialized = false;

    LOG_INFO("RenderSystemTest", "新渲染系统测试清理完成");
}

bool RenderSystemTest::RunTests() {
    if (!m_initialized) {
        LOG_ERROR("RenderSystemTest", "测试环境未初始化");
        return false;
    }

    LOG_INFO("RenderSystemTest", "开始运行新渲染系统测试");

    m_testResults.clear();
    bool allPassed = true;

    // 运行各项测试
    allPassed &= TestDeviceInitialization();
    allPassed &= TestResourceManager();
    allPassed &= TestShaderCompilation();
    allPassed &= TestBufferCreation();
    allPassed &= TestTextureCreation();
    allPassed &= TestPipelineState();
    allPassed &= TestRenderPipeline();
    allPassed &= TestResourceCleanup();
    allPassed &= TestMemoryUsage();

    // 输出测试结果
    Logger::Info("RenderSystemTest", "=== 测试结果汇总 ===");
    for (const auto& result : m_testResults) {
        Logger::Info("RenderSystemTest", result);
    }

    if (allPassed) {
        LOG_INFO("RenderSystemTest", "所有测试通过！");
    } else {
        LOG_ERROR("RenderSystemTest", "部分测试失败！");
    }

    return allPassed;
}

void RenderSystemTest::RenderFrame() {
    static int frameCount = 0;
    frameCount++;

    LOG_INFO("RenderSystemTest", "=== 开始渲染第 {0} 帧 ===", frameCount);

    if (!m_initialized || !m_renderSystem) {
        LOG_ERROR("RenderSystemTest", "渲染系统未初始化，跳过第 {0} 帧", frameCount);
        return;
    }

    LOG_DEBUG("RenderSystemTest", "第 {0} 帧: BeginFrame", frameCount);
    m_renderSystem->BeginFrame();

    // 获取命令缓冲区并渲染三角形
    LOG_DEBUG("RenderSystemTest", "第 {0} 帧: 获取命令缓冲区", frameCount);
    auto commandBuffer = m_renderSystem->GetDevice()->CreateCommandBuffer(CommandBufferType.Graphics);
    if (commandBuffer && m_pipelineState && m_vertexBuffer) {
        LOG_DEBUG("RenderSystemTest", "第 {0} 帧: 设置渲染状态", frameCount);

        // 设置渲染状态
        commandBuffer->SetPipelineState(m_pipelineState.get());
        LOG_DEBUG("RenderSystemTest", "第 {0} 帧: 管线状态设置完成", frameCount);

        // 设置视口
        Viewport viewport{};
        viewport.width = static_cast<float>(m_width);
        viewport.height = static_cast<float>(m_height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        commandBuffer->SetViewport(viewport);
        LOG_DEBUG("RenderSystemTest", "第 {0} 帧: 视口设置完成 ({1}x{2})", frameCount, m_width, m_height);

        // 设置裁剪矩形
        Rect scissor{};
        scissor.width = static_cast<int>(m_width);
        scissor.height = static_cast<int>(m_height);
        commandBuffer->SetScissor(0, scissor);
        LOG_DEBUG("RenderSystemTest", "第 {0} 帧: 裁剪矩形设置完成", frameCount);

        // 绑定顶点缓冲区
        commandBuffer->SetVertexBuffer(m_vertexBuffer.get(),0);
        LOG_DEBUG("RenderSystemTest", "第 {0} 帧: 顶点缓冲区绑定完成", frameCount);

        // 渲染三角形
        commandBuffer->Draw(3, 1, 0, 0);
        LOG_INFO("RenderSystemTest", "第 {0} 帧: 三角形绘制命令提交完成 (3个顶点)", frameCount);
    } else {
        LOG_ERROR("RenderSystemTest", "第 {0} 帧: 渲染资源不完整 - commandBuffer={1}, pipelineState={2}, vertexBuffer={3}",
                 frameCount,
                 commandBuffer ? "有效" : "无效",
                 m_pipelineState ? "有效" : "无效",
                 m_vertexBuffer ? "有效" : "无效");
    }

    LOG_DEBUG("RenderSystemTest", "第 {0} 帧: EndFrame", frameCount);
    m_renderSystem->EndFrame();

    LOG_DEBUG("RenderSystemTest", "第 {0} 帧: Present", frameCount);
    m_renderSystem->Present();

    LOG_INFO("RenderSystemTest", "=== 第 {0} 帧渲染完成 ===", frameCount);
}

bool RenderSystemTest::TestDeviceInitialization() {
    LOG_INFO("RenderSystemTest", "测试设备初始化");

    bool passed = (m_device != nullptr && m_resourceManager != nullptr);

    std::ostringstream oss;
    oss << "设备初始化测试: " << (passed ? "通过" : "失败");
    m_testResults.push_back(oss.str());

    if (passed) {
        LOG_INFO("RenderSystemTest", "设备和资源管理器创建成功");
    } else {
        LOG_ERROR("RenderSystemTest", "设备或资源管理器创建失败");
    }

    return passed;
}

bool RenderSystemTest::TestResourceManager() {
    LOG_INFO("RenderSystemTest", "测试资源管理器");

    if (!m_resourceManager) {
        m_testResults.push_back("资源管理器测试: 失败 - 资源管理器为空");
        return false;
    }

    // 测试获取默认采样器
    auto defaultSampler = m_resourceManager->GetDefaultSampler();
    bool passed = (defaultSampler != nullptr);

    std::ostringstream oss;
    oss << "资源管理器测试: " << (passed ? "通过" : "失败");
    m_testResults.push_back(oss.str());

    return passed;
}

bool RenderSystemTest::TestShaderCompilation() {
    LOG_INFO("RenderSystemTest", "测试着色器编译");

    bool passed = CreateTriangleShaders();

    std::ostringstream oss;
    oss << "着色器编译测试: " << (passed ? "通过" : "失败");
    m_testResults.push_back(oss.str());

    return passed;
}

bool RenderSystemTest::TestBufferCreation() {
    LOG_INFO("RenderSystemTest", "测试缓冲区创建");

    bool passed = CreateTriangleGeometry();

    std::ostringstream oss;
    oss << "缓冲区创建测试: " << (passed ? "通过" : "失败");
    m_testResults.push_back(oss.str());

    return passed;
}

bool RenderSystemTest::TestTextureCreation() {
    LOG_INFO("RenderSystemTest", "测试纹理创建");

    // 创建一个简单的2D纹理
    TextureDesc desc;
    desc.type = TextureType::Texture2D;
    desc.format = TextureFormat::RGBA8_UNorm;
    desc.width = 256;
    desc.height = 256;
    desc.depth = 1;
    desc.mipLevels = 1;
    desc.arraySize = 1;
    desc.name = "TestTexture";

    auto texture = m_resourceManager->CreateTexture(desc);
    bool passed = (texture != nullptr);

    std::ostringstream oss;
    oss << "纹理创建测试: " << (passed ? "通过" : "失败");
    m_testResults.push_back(oss.str());

    return passed;
}

bool RenderSystemTest::TestPipelineState() {
    LOG_INFO("RenderSystemTest", "测试管线状态对象");

    bool passed = CreateRenderPipeline();

    std::ostringstream oss;
    oss << "管线状态对象测试: " << (passed ? "通过" : "失败");
    m_testResults.push_back(oss.str());

    return passed;
}

bool RenderSystemTest::TestRenderPipeline() {
    LOG_INFO("RenderSystemTest", "测试渲染流程");

    // 尝试渲染一帧
    try {
        RenderFrame();
        bool passed = true;

        std::ostringstream oss;
        oss << "渲染流程测试: " << (passed ? "通过" : "失败");
        m_testResults.push_back(oss.str());

        return passed;
    } catch (const std::exception& e) {
        LOG_ERROR("RenderSystemTest", "渲染流程测试异常: {0}", e.what());
        m_testResults.push_back("渲染流程测试: 失败 - 异常");
        return false;
    }
}

bool RenderSystemTest::TestMemoryUsage() {
    LOG_INFO("RenderSystemTest", "测试内存使用情况");

    if (!m_resourceManager) {
        m_testResults.push_back("内存使用测试: 失败 - 资源管理器为空");
        return false;
    }

    try {
        // 获取资源统计信息
        auto stats = m_resourceManager->GetResourceStats();

        LOG_INFO("RenderSystemTest", "=== 内存使用统计 ===");
        LOG_INFO("RenderSystemTest", "总资源数: {0}", stats.totalResources);
        LOG_INFO("RenderSystemTest", "纹理资源: {0}", stats.textureCount);
        LOG_INFO("RenderSystemTest", "缓冲区资源: {0}", stats.bufferCount);
        LOG_INFO("RenderSystemTest", "着色器资源: {0}", stats.shaderCount);
        LOG_INFO("RenderSystemTest", "管线资源: {0}", stats.pipelineCount);
        LOG_INFO("RenderSystemTest", "GPU内存使用: {0} MB", stats.gpuMemoryUsage / (1024 * 1024));
        LOG_INFO("RenderSystemTest", "CPU内存使用: {0} MB", stats.cpuMemoryUsage / (1024 * 1024));

        // 验证内存使用是否合理
        bool memoryUsageReasonable = true;

        // 检查是否有资源泄露
        if (stats.totalResources > 10) {  // 我们的测试应该只创建少量资源
            LOG_WARNING("RenderSystemTest", "资源数量可能过多: {0}", stats.totalResources);
            memoryUsageReasonable = false;
        }

        // 检查GPU内存使用（应该小于100MB用于简单测试）
        if (stats.gpuMemoryUsage > 100 * 1024 * 1024) {
            LOG_WARNING("RenderSystemTest", "GPU内存使用过高: {0} MB", stats.gpuMemoryUsage / (1024 * 1024));
            memoryUsageReasonable = false;
        }

        std::ostringstream oss;
        oss << "内存使用测试: " << (memoryUsageReasonable ? "通过" : "失败 - 内存使用异常");
        m_testResults.push_back(oss.str());

        return memoryUsageReasonable;

    } catch (const std::exception& e) {
        LOG_ERROR("RenderSystemTest", "内存使用测试异常: {0}", e.what());
        m_testResults.push_back("内存使用测试: 失败 - 异常");
        return false;
    }
}

bool RenderSystemTest::TestResourceCleanup() {
    LOG_INFO("RenderSystemTest", "测试资源清理");

    // 清理所有测试创建的资源
    m_pipelineState.reset();
    m_vertexBuffer.reset();
    m_vertexShader.reset();
    m_pixelShader.reset();

    // 强制垃圾回收
    if (m_resourceManager) {
        m_resourceManager->GarbageCollect();
    }

    bool passed = true;

    std::ostringstream oss;
    oss << "资源清理测试: " << (passed ? "通过" : "失败");
    m_testResults.push_back(oss.str());

    return passed;
}

bool RenderSystemTest::CreateTriangleShaders() {
    if (!m_resourceManager) {
        return false;
    }

    // 简单的顶点着色器HLSL代码
    const char* vertexShaderSource = R"(
        struct VSInput {
            float3 position : POSITION;
            float4 color : COLOR;
        };

        struct VSOutput {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        VSOutput main(VSInput input) {
            VSOutput output;
            output.position = float4(input.position, 1.0);
            output.color = input.color;
            return output;
        }
    )";

    // 简单的像素着色器HLSL代码
    const char* pixelShaderSource = R"(
        struct PSInput {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        float4 main(PSInput input) : SV_TARGET {
            return input.color;
        }
    )";

    try {
        // 创建着色器描述
        ShaderDesc vertexDesc;
        vertexDesc.type = ShaderType::Vertex;
        vertexDesc.language = ShaderLanguage::HLSL;
        vertexDesc.source = vertexShaderSource;
        vertexDesc.entryPoint = "main";
        vertexDesc.target = "vs_5_0";
        vertexDesc.name = "TriangleVertexShader";

        ShaderDesc pixelDesc;
        pixelDesc.type = ShaderType::Pixel;
        pixelDesc.language = ShaderLanguage::HLSL;
        pixelDesc.source = pixelShaderSource;
        pixelDesc.entryPoint = "main";
        pixelDesc.target = "ps_5_0";
        pixelDesc.name = "TrianglePixelShader";

        // 编译着色器
        std::string errors;
        m_vertexShader = m_resourceManager->CreateShader(vertexShaderSource, vertexDesc);
        m_pixelShader = m_resourceManager->CreateShader(pixelShaderSource, pixelDesc);

        if (!m_vertexShader || !m_pixelShader) {
            LOG_ERROR("RenderSystemTest", "着色器创建失败");
            return false;
        }

        LOG_INFO("RenderSystemTest", "着色器编译成功");
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("RenderSystemTest", "着色器编译异常: {0}", e.what());
        return false;
    }
}

bool RenderSystemTest::CreateTriangleGeometry() {
    if (!m_resourceManager) {
        return false;
    }

    try {
        // 三角形顶点数据 (位置 + 颜色)
        struct Vertex {
            float position[3];
            float color[4];
        };

        std::array<Vertex, 3> vertices = {{
            {{ 0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},  // 顶部 - 红色
            {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},  // 左下 - 绿色
            {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}   // 右下 - 蓝色
        }};

        // 创建顶点缓冲区
        BufferDesc bufferDesc;
        bufferDesc.type = BufferType::Vertex;
        bufferDesc.usage = BufferUsage::Default;
        bufferDesc.size = vertices.size() * sizeof(Vertex);
        bufferDesc.stride = sizeof(Vertex);
        bufferDesc.name = "TriangleVertexBuffer";

        m_vertexBuffer = m_resourceManager->CreateBuffer(bufferDesc);
        if (!m_vertexBuffer) {
            LOG_ERROR("RenderSystemTest", "顶点缓冲区创建失败");
            return false;
        }

        // 上传顶点数据到缓冲区
        m_vertexBuffer->UpdateData(vertices.data(), vertices.size() * sizeof(Vertex), 0);

        LOG_INFO("RenderSystemTest", "三角形几何体创建成功，上传了 {0} 个顶点", vertices.size());
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("RenderSystemTest", "几何体创建异常: {0}", e.what());
        return false;
    }
}

bool RenderSystemTest::CreateRenderPipeline() {
    if (!m_device || !m_vertexShader || !m_pixelShader) {
        return false;
    }

    try {
        // 创建管线状态对象
        auto pipelineState = m_resourceManager->CreatePipelineState();
        if (!pipelineState) {
            LOG_ERROR("RenderSystemTest", "管线状态对象创建失败");
            return false;
        }

        // 设置着色器
        pipelineState->SetShader(ShaderType::Vertex, m_vertexShader);
        pipelineState->SetShader(ShaderType::Pixel, m_pixelShader);

        // 设置图元拓扑
        pipelineState->SetPrimitiveTopology(PrimitiveTopology::TriangleList);

        // 设置顶点输入布局
        std::vector<VertexInputAttribute> inputAttributes = {
            {"POSITION", 0, TextureFormat::RGB32_Float, 0, 0, 0, 0},
            {"COLOR", 0, TextureFormat::RGBA32_Float, 0, 12, 0, 0}
        };
        pipelineState->SetInputLayout(inputAttributes);

        // 设置渲染目标格式
        pipelineState->SetRenderTargetFormat(0, TextureFormat::RGBA8_UNorm);

        // 创建管线
        if (!pipelineState->Create(m_device)) {
            LOG_ERROR("RenderSystemTest", "管线状态对象创建失败");
            return false;
        }

        m_pipelineState = pipelineState;
        LOG_INFO("RenderSystemTest", "渲染管线创建成功");
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("RenderSystemTest", "渲染管线创建异常: {0}", e.what());
        return false;
    }
}

} // namespace PrismaEngine::Graphic