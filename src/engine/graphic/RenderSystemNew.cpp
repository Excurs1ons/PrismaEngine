#include "RenderSystemNew.h"
#include "../Logger.h"
#include "../SceneManager.h"
#include "../Camera.h"
#include "../core/ECS.h"
#include "pipelines/forward/ForwardPipeline.h"

#ifdef PRISMA_ENABLE_RENDER_DX12
// DirectX12适配器
#include "adapters/dx12/DX12Adapters.h"
#endif

#ifdef PRISMA_ENABLE_RENDER_VULKAN
// DirectX12适配器
//#include "adapters/vulkan/VulkanAdapters.h"
#endif

#if defined(_WIN32)
#include <Windows.h>
extern HWND g_hWnd;
#endif

namespace PrismaEngine
{ namespace Graphic {

        bool RenderSystem::Initialize() {
            // 使用默认描述初始化
            RenderSystemDesc defaultDesc;
            return Initialize(defaultDesc);
        }

        bool RenderSystem::Initialize(const RenderSystemDesc &desc) {
            LOG_INFO("Render", "正在初始化新的渲染系统");
            m_desc = desc;

            // 1. 初始化设备
            if (!InitializeDevice(desc)) {
                LOG_ERROR("Render", "设备初始化失败");
                return false;
            }

            // 2. 初始化资源管理器
            if (!InitializeResourceManager()) {
                LOG_ERROR("Render", "资源管理器初始化失败");
                return false;
            }

            // 3. 创建适配器层（桥接新旧接口）
            if (!CreateAdapters()) {
                LOG_ERROR("Render", "适配器层创建失败");
                return false;
            }

            // 4. 初始化渲染流程
            if (!InitializePipelines()) {
                LOG_ERROR("Render", "渲染流程初始化失败");
                return false;
            }

            // 5. 获取并设置默认渲染目标
            // TODO: 从新设备获取渲染目标
            // 暂时跳过，后续实现

            LOG_INFO("Render", "新渲染系统初始化完成");
            return true;
        }

        void RenderSystem::Shutdown() {
            LOG_INFO("Render", "正在关闭渲染系统");

            // 等待渲染线程完成
            if (m_renderThread.IsRunning()) {
                m_renderThread.Stop();
                m_renderThread.Join();
            }

            // 清理资源
            m_mainPipeline.reset();
            m_forwardPipeline.reset();
            //m_legacyPipeline.reset();
            m_resourceManager.reset();
            m_device.reset();

            LOG_INFO("Render", "渲染系统已关闭");
        }

        RenderSystem::~RenderSystem() {
            // 默认析构函数会自动清理 std::unique_ptr 成员
            // 由于这是一个显式定义的析构函数，编译器会在看到 ForwardPipeline 完整定义的地方生成代码
        }

        void RenderSystem::Update(float deltaTime) {
            UpdateStats(deltaTime);

            // 如果没有使用渲染线程，直接渲染
            if (!m_renderThread.IsRunning()) {
                RenderFrame();
            }
        }

        void RenderSystem::BeginFrame() {
            if (m_device) {
                m_device->BeginFrame();
            }
        }

        void RenderSystem::EndFrame() {
            if (m_device) {
                m_device->EndFrame();
            }
        }

        void RenderSystem::Present() {
            if (m_device) {
                m_device->Present();
            }

            // 更新帧统计
            m_stats.frameCount++;
        }

        void RenderSystem::Resize(uint32_t width, uint32_t height) {
            LOG_INFO("Render", "调整渲染目标大小: {0}x{1}", width, height);

            // TODO: 实现新设备的Resize功能
            // if (m_device) {
            //     m_device->GetSwapChain()->ResizeBuffers(...);
            // }

            // 更新描述
            m_desc.width = width;
            m_desc.height = height;
        }

        void RenderSystem::SetMainPipeline(std::shared_ptr <IPipeline> pipeline) {
            m_mainPipeline = pipeline;
            if (pipeline && m_device) {
                pipeline->Initialize(m_device.get());
            }
        }

        void RenderSystem::SetGuiRenderCallback(GuiRenderCallback callback) {
            m_guiCallback = callback;
        }

        RenderSystem::RenderStats RenderSystem::GetRenderStats() const {
            RenderStats stats = m_stats;

            // 获取设备统计信息
            if (m_device) {
                auto deviceStats = m_device->GetRenderStats();
                stats.drawCalls = deviceStats.drawCalls;
                stats.triangles = deviceStats.triangles;
            }

            // 获取内存使用情况
            if (m_device) {
                auto memInfo = m_device->GetGPUMemoryInfo();
                stats.gpuMemoryUsage = memInfo.usedMemory;
            }

            if (m_resourceManager) {
                auto resourceStats = m_resourceManager->GetResourceStats();
                stats.cpuMemoryUsage = resourceStats.totalMemoryUsage;
            }

            return stats;
        }

        void RenderSystem::ResetStats() {
            m_stats = {};
            m_stats.frameCount = 0;
        }

        bool RenderSystem::InitializeDevice(const RenderSystemDesc &desc) {
            LOG_INFO("Render", "初始化渲染设备，后端类型: {0}", static_cast<int>(desc.backendType));

            // 直接创建新的适配器设备
            switch (desc.backendType) {
                case RenderAPIType::DirectX12: {
#if defined(PRISMA_ENABLE_RENDER_DX12) || (defined(PRISMA_PLATFORM_WINDOWS) && !defined(PRISMA_FORCE_GLM))
                    // 准备设备描述
                    DeviceDesc deviceDesc;
                    deviceDesc.name = desc.name;
                    deviceDesc.windowHandle = desc.windowHandle;
                    deviceDesc.width = desc.width;
                    deviceDesc.height = desc.height;
                    deviceDesc.enableDebug = desc.enableDebug;
                    deviceDesc.enableValidation = desc.enableValidation;
                    deviceDesc.maxFramesInFlight = desc.maxFramesInFlight;

                    // 使用工厂创建DX12设备
                    m_device = DX12::CreateDX12RenderDeviceInterface(deviceDesc);
                    if (!m_device) {
                        LOG_ERROR("Render", "DX12设备创建失败");
                        return false;
#endif
                }
                    break;
#if defined(PRISMA_ENABLE_RENDER_VULKAN)
                    case RenderAPIType::Vulkan: {
                        // TODO: 实现Vulkan工厂
                        LOG_ERROR("Render", "Vulkan后端暂未实现");
                        return false;
#endif
            }
            default: {
                LOG_ERROR("Render", "不支持的渲染后端类型: {0}",
                          static_cast<int>(desc.backendType));
                return false;
            }
        }

        if (!m_device) {
        LOG_ERROR("Render", "渲染设备创建失败");
        return false;
    }

    LOG_INFO("Render", "设备初始化完成");
    return true;
}

bool RenderSystem::InitializeResourceManager() {
    if (!m_device) {
        LOG_ERROR("Render", "设备未初始化，无法创建资源管理器");
        return false;
    }

    // 这里应该创建具体的ResourceManager实现
    // 暂时返回true，在下一个任务中实现
    LOG_INFO("Render", "资源管理器初始化完成（待实现）");
    return true;
}

bool RenderSystem::InitializePipelines() {
    // 初始化旧的渲染管线（兼容性）
    //m_legacyPipeline = std::make_unique<ScriptableRenderPipeline>();
    // TODO: 需要重新设计管线系统以适配新架构
    // 临时跳过初始化
    // if (!m_legacyPipeline->Initialize(m_legacyBackend.get())) {
    //     LOG_ERROR("Render", "可编程渲染管线初始化失败");
    //     return false;
    // }

    // 初始化前向渲染管线
    m_forwardPipeline = std::make_unique<PrismaEngine::Graphic::ForwardPipeline>();
    // TODO: 需要重新设计以适配新架构
    // 临时跳过
    // if (!m_forwardPipeline->Initialize(m_legacyPipeline.get())) {
    //     LOG_ERROR("Render", "前向渲染管线初始化失败");
    //     return false;
    // }

    // 创建新的主渲染流程（使用旧流程作为临时实现）
    // 这里可以创建基于新接口的渲染流程
    // SetMainPipeline(...);

    LOG_INFO("Render", "渲染流程初始化完成");
    return true;
}

bool RenderSystem::CreateAdapters() {
    // 这个方法在InitializeDevice中已经处理
    // 这里可以做额外的适配器配置
    return true;
}

void RenderSystem::RenderFrame() {
    if (!m_device) {
        return;
    }

    // TODO: 使用新的设备接口实现渲染
    // 这里是使用新接口的示例流程：

    // 1. 开始帧
    m_device->BeginFrame();

    // 2. 渲染场景
    // TODO: 实现场景渲染逻辑

    // 3. GUI渲染
    // TODO: 实现GUI渲染

    // 4. 结束帧
    m_device->EndFrame();

    // 5. 呈现
    m_device->Present();
}

void RenderSystem::UpdateStats(float deltaTime) {
    m_stats.frameTime = deltaTime;

    // 计算FPS（每秒更新一次）
    static float fpsAccumulator = 0.0f;
    static uint32_t fpsFrameCount = 0;
    static float fpsUpdateTime = 0.0f;

    fpsAccumulator += 1.0f / deltaTime;
    fpsFrameCount++;
    fpsUpdateTime += deltaTime;

    if (fpsUpdateTime >= 1.0f) {
        m_stats.fps = fpsAccumulator / fpsFrameCount;
        fpsAccumulator = 0.0f;
        fpsFrameCount = 0;
        fpsUpdateTime = 0.0f;
    }
}

RenderContext RenderSystem::GetRenderContext() const {
    RenderContext context;
    context.device = m_device.get();
    context.frameIndex = m_stats.frameCount;
    context.deltaTime = m_stats.frameTime;
    context.renderTargetWidth = m_desc.width;
    context.renderTargetHeight = m_desc.height;

    // 添加场景数据
    auto &world = ::PrismaEngine::Core::ECS::World::GetInstance();
    auto sceneManager = static_cast<::PrismaEngine::SceneManager *>(world.GetSystem<::PrismaEngine::SceneManager>());
    if (sceneManager) {
        auto activeScene = sceneManager->GetCurrentScene();
        if (activeScene) {
            auto camera = activeScene->GetMainCamera();
            if (camera) {
                context.sceneData = static_cast<void *>(camera.get());
            }
        }
    }

    return context;
}

}
} // namespace PrismaEngine::Graphic
