#include "ApplicationEditor.h"
#include "Logger.h"
#include "PlatformSDL.h"
#include "RenderBackendVulkan.h"
#include "RenderSystem.h"
#include <iostream>
#include <stdexcept>
#include <windows.h>

#include "SceneManager.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "nlohmann/json.hpp"
#include <vector>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>

using namespace Engine;

static VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
/// @brief 日志输出测试演示
void ShowDemo() {
    LOG_INFO("Demo", "这是一条信息消息");
    LOG_WARNING("Demo", "这是一条警告消息");
    LOG_ERROR("Demo", "这是一条错误消息");
    LOG_FATAL("Demo", "这是一条致命错误消息");
    LOG_DEBUG("Demo", "这是一条调试消息");
    LOG_TRACE("Demo", "这是一条跟踪消息");
}

ApplicationEditor::ApplicationEditor() {
    LOG_INFO("Editor", "正在初始化编辑器");
}

ApplicationEditor::~ApplicationEditor()
{
    LOG_INFO("Editor", "正在关闭编辑器");
}

bool ApplicationEditor::Initialize()
{
    LOG_INFO("Editor", "正在初始化编辑器");
    
    // 1. 初始化平台 (SDL)
    auto platform = PlatformSDL::GetInstance();
    if (!platform->Initialize()) {
        LOG_FATAL("System", "平台初始化失败");
        return false;
    }
    
    // 2. 创建窗口
    WindowProps props("SDL3 Editor", 1600, 900);
    // props.FullScreenMode = FullScreenMode::Window; 
    // props.ShowState = WindowShowState::Maximize;

    auto window = platform->CreateWindow(props);

    if (!window) {
        LOG_FATAL("System", "无法创建窗口");
        return false;
    }
    
    // 3. 初始化渲染系统 (Vulkan)
    auto renderSystem = RenderSystem::GetInstance();
    if (!renderSystem->Initialize(platform.get(), RenderBackendType::Vulkan, window, nullptr, props.Width, props.Height)) {
        LOG_FATAL("System", "渲染系统初始化失败");
        return false;
    }

    // 初始化场景管理器
    if (!SceneManager::GetInstance()->Initialize()) {
        LOG_FATAL("System", "场景管理器初始化失败");
        return false;
    }

    LOG_INFO("Editor", "编辑器初始化完成");

    // 4. 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    ImGui::StyleColorsDark();

    // 自动 DPI 缩放
    float dpiScale = SDL_GetWindowDisplayScale((SDL_Window*)window);
    if (dpiScale > 1.0f) {
        // 加载高分辨率字体以获得更清晰的渲染效果
        float baseFontSize = 16.0f;
        io.Fonts->Clear(); // 先清除默认字体
        float dpiScaleNormalized = (dpiScale - 1.0f) / 2.0f + 1.0f;
        ImFontConfig fontConfig;
        fontConfig.SizePixels = baseFontSize * dpiScaleNormalized;  // 加载高分辨率字体
        //fontConfig.OversampleH = 2;  // 水平过采样，提高清晰度
        //fontConfig.OversampleV = 2;  // 垂直过采样，提高清晰度
        io.Fonts->AddFontDefault(&fontConfig);
        
        // 缩放 UI 元素尺寸
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(dpiScaleNormalized);
        
        // 不使用 FontGlobalScale，因为字体已经是高分辨率的
        //io.FontGlobalScale = 1.0f;
    }

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // 初始化 ImGui 后端
    // 获取 RenderBackend 并转换为 RendererVulkan
    auto backend = dynamic_cast<RendererVulkan*>(renderSystem->GetRenderBackend());
    if (!backend) {
        LOG_ERROR("Editor", "ImGui 初始化失败：无法获取 Vulkan 后端");
        return false;
    }

    // 创建 Descriptor Pool
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets                    = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount              = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes                 = pool_sizes;

    if (vkCreateDescriptorPool(backend->GetDevice(), &pool_info, nullptr, &g_DescriptorPool) != VK_SUCCESS) {
        LOG_ERROR("Editor", "ImGui 初始化失败：无法创建 Descriptor Pool");
        return false;
    }

    // 初始化 ImGui SDL3
    if (!ImGui_ImplSDL3_InitForVulkan((SDL_Window*)window)) {
        LOG_ERROR("Editor", "ImGui SDL3 初始化失败");
        return false;
    }

    // 注册事件回调
    platform->SetEventCallback([](const SDL_Event* event) -> bool {
        ImGui_ImplSDL3_ProcessEvent(event);
        
        if (event->type == SDL_EVENT_WINDOW_RESIZED) {
            RenderSystem::GetInstance()->Resize(event->window.data1, event->window.data2);
        }

        return false;
    });

    // 初始化 ImGui Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance                  = backend->GetVulkanInstance();
    init_info.PhysicalDevice            = backend->GetPhysicalDevice();
    init_info.Device                    = backend->GetDevice();
    init_info.QueueFamily               = backend->GetGraphicsQueueFamily();
    init_info.Queue                     = backend->GetGraphicsQueue();
    init_info.PipelineCache             = VK_NULL_HANDLE;
    init_info.DescriptorPool            = g_DescriptorPool;
    init_info.Subpass                   = 0;
    init_info.MinImageCount             = backend->GetMinImageCount();
    init_info.ImageCount                = backend->GetImageCount();
    init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator                 = nullptr;
    init_info.CheckVkResultFn           = nullptr;
    init_info.RenderPass                = backend->GetRenderPass();
    if (!ImGui_ImplVulkan_Init(&init_info)) {
        LOG_ERROR("Editor", "ImGui Vulkan 初始化失败");
        ImGui_ImplSDL3_Shutdown();
        return false;
    }

    // 上传字体
    {
        // 这里需要一个临时的 CommandBuffer，或者使用 backend 提供的机制
        // 由于 backend 没有暴露创建 CommandBuffer 的接口，我们可能需要 backend 提供一个 Helper
        // 或者我们直接使用 backend 的 GraphicsQueue 和 CommandPool (如果暴露了)
        // 简单起见，我们假设 backend 已经初始化完成，我们可以创建一个临时的 CommandPool 和 Buffer
        // 但这太麻烦了。
        // 更好的方式是让 backend 提供一个 "ExecuteOneTimeCommand" 的接口。
        // 但现在我们只能利用现有的。
        // 实际上，ImGui_ImplVulkan_CreateFontsTexture 需要一个 CommandBuffer。
        // 我们可以暂时跳过字体上传，等到第一帧渲染时再做？不行。
        
        // 让我们在 RenderBackendVulkan 中添加一个 ExecuteImmediateCommand 接口？
        // 或者，我们直接在 ApplicationEditor 中创建 CommandPool。
        // 我们有 Device 和 QueueFamily。
        
        VkCommandPool commandPool;
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = backend->GetGraphicsQueueFamily();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        
        if (vkCreateCommandPool(backend->GetDevice(), &poolInfo, nullptr, &commandPool) == VK_SUCCESS) {
            VkCommandBuffer commandBuffer;
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = commandPool;
            allocInfo.commandBufferCount = 1;
            
            vkAllocateCommandBuffers(backend->GetDevice(), &allocInfo, &commandBuffer);
            
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            
            vkBeginCommandBuffer(commandBuffer, &beginInfo);
            ImGui_ImplVulkan_CreateFontsTexture();
            vkEndCommandBuffer(commandBuffer);
            
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            
            vkQueueSubmit(backend->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(backend->GetGraphicsQueue());
            
            vkDestroyCommandPool(backend->GetDevice(), commandPool, nullptr);
            // ImGui_ImplVulkan_DestroyFontUploadObjects(); // 已移除
        }
    }

    // 注册渲染回调
    // 使用 [=] 捕获列表防止 lambda 转换为函数指针，消除 std::function 构造时的歧义
    Engine::RenderSystem::GuiRenderCallback callback = [=](void* cmdBuffer) {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), (VkCommandBuffer)cmdBuffer);
    };
    renderSystem->SetGuiRenderCallback(callback);
    
	return true;
}

int ApplicationEditor::Run()
{
    auto platform = PlatformSDL::GetInstance();
    auto renderSystem = RenderSystem::GetInstance();
    bool running = true;

    while (running) {
        platform->PumpEvents();
        if (platform->ShouldClose(nullptr)) {
            running = false;
        }

        // 开始新的一帧
        renderSystem->BeginFrame();
        
        // ImGui 新帧
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 构建 UI
        ShowDemo();
        ImGui::ShowDemoWindow();
        
        // 渲染 ImGui
        ImGui::Render();
        // renderSystem->RenderImGui(); // 已通过回调处理
        
        // 更新和渲染额外的平台窗口
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        // 结束帧并呈现
        renderSystem->EndFrame();
        renderSystem->Present();
    }
    return 0;
}

void ApplicationEditor::Shutdown()
{
    LOG_INFO("Editor", "正在关闭编辑器");
    
    auto renderSystem = RenderSystem::GetInstance();
    auto backend = dynamic_cast<RendererVulkan*>(renderSystem->GetRenderBackend());
    if (backend) {
        vkDeviceWaitIdle(backend->GetDevice());
    }

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    
    if (g_DescriptorPool != VK_NULL_HANDLE && backend) {
        vkDestroyDescriptorPool(backend->GetDevice(), g_DescriptorPool, nullptr);
        g_DescriptorPool = VK_NULL_HANDLE;
    }

    // 先关闭 SDL，避免 DXGI 冲突
    PlatformSDL::GetInstance()->Shutdown();

    renderSystem->Shutdown();
}

// 实现导出函数
extern "C" {
    __declspec(dllexport) bool InitializeEditor() {
        return ApplicationEditor::GetInstance().Initialize();
    }

    __declspec(dllexport) int RunEditor() {
        return ApplicationEditor::GetInstance().Run();
    }

    __declspec(dllexport) void ShutdownEditor() {
        ApplicationEditor::GetInstance().Shutdown();
    }
}