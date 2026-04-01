#include "Editor.h"
#include "EditorLayer.h"
#include "../engine/Engine.h"
#include "../engine/Platform.h"
#include "../engine/graphic/RenderSystem.h"
#include "../engine/SceneManager.h"
#include "../engine/Scene.h"
#include "CommandLineEditor.h"
#include "CommandLineParser.h"
#include "Environment.h"
#include "graphic/ImGuiVulkanResourceManager.h"
#include <filesystem>

// ImGui
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

// 访问具体的 Vulkan 设备类型，以注册 ImGui 渲染回调
#include "graphic/adapters/vulkan/RenderDeviceVulkan.h"
#include "graphic/adapters/vulkan/VulkanResources.h"


// [修复] 移除 IMGUI_IMPL_VULKAN_USE_LOADER 定义
// 原因：当定义此宏时，ImGui 会使用动态函数指针调用 Vulkan API。
//        如果这些函数指针未被正确加载，调用时会访问空指针导致 0xc0000005 异常。
// 解决：使用标准 Vulkan 函数原型（通过 Vulkan-Headers 提供），避免函数指针初始化问题。
// #define IMGUI_IMPL_VULKAN_USE_LOADER

namespace Prisma {

Editor::Editor() : Application(ApplicationSpecification{"Prisma Editor", 1280, 720}) {}

Editor::~Editor() {}

int Editor::OnInitialize() {
    LOG_INFO("Editor", "正在初始化编辑器插件 (纯净模式)...");

    // 1. 初始化 ImGui
    int result = OnImGuiInitialize();
    if (result != 0) {
        LOG_ERROR("Editor", "ImGui 初始化失败");
        return result;
    }

    // 2. 推送编辑器层
    PushLayer(new EditorLayer());

    LOG_INFO("Editor", "编辑器插件初始化成功。");
    return 0;
}

int Editor::OnImGuiInitialize() {
    LOG_INFO("Editor", "OnImGuiInitialize: 正在启动...");
    IMGUI_CHECKVERSION();
    LOG_INFO("Editor", "OnImGuiInitialize: 创建上下文...");
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();

    // [改动] 加载中文字体以解决编辑器无法显示中文的问题
    ImFont* font = nullptr;
    std::vector<std::string> fontPaths = {
        "C:/Windows/Fonts/msyh.ttc",   // Microsoft YaHei
        "C:/Windows/Fonts/msyh.ttf",
        "C:/Windows/Fonts/simsun.ttc", // SimSun
        "assets/fonts/msyh.ttc"
    };

    for (const auto& path : fontPaths) {
        if (std::filesystem::exists(path)) {
            // 使用 GetGlyphRangesChineseSimplifiedCommon() 获取常用中文字符集
            font = io.Fonts->AddFontFromFileTTF(path.c_str(), 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
            if (font) {
                LOG_INFO("Editor", "成功加载中文字体: %s", path.c_str());
                break;
            }
        }
    }

    if (!font) {
        LOG_WARN("Editor", "加载中文字体失败。将使用 ImGui 默认字体。");
    }

    auto& engine      = Engine::Get();
    auto renderSystem = engine.GetRenderSystem();
    auto device       = renderSystem->GetDevice();
    auto* vkDevice    = static_cast<Prisma::Graphic::Vulkan::RenderDeviceVulkan*>(device);
    
    LOG_INFO("Editor", "OnImGuiInitialize: 正在绑定 SDL3...");
    // 绑定后端
    auto& window          = engine.GetWindow();
    SDL_Window* sdlWindow = static_cast<SDL_Window*>(window.GetNativeWindow());
    if (!sdlWindow) {
        LOG_ERROR("Editor", "原生窗口为空！");
        return -1;
    }
    
    if (!ImGui_ImplSDL3_InitForVulkan(sdlWindow)) {
        LOG_ERROR("Editor", "ImGui_ImplSDL3_InitForVulkan 失败！");
        return -1;
    }

    LOG_INFO("Editor", "OnImGuiInitialize: 正在初始化 Vulkan 后端...");
    ImGui_ImplVulkan_InitInfo init_info = {};

    // [修复] ApiVersion 必须与创建 VkInstance 时使用的 Vulkan API 版本一致。
    //   目的：ImGui 内部根据此版本决定初始化路径（例如是否启用某些 Vulkan 1.2/1.3 特性）。
    //   问题根源：原代码未设置此字段，默认值为 0，导致 ImGui_ImplVulkan_Init 内部
    //             选择了错误的代码路径，字体纹理的 DescriptorSet 未被正确创建，
    //             其 TexID 留在 ImTextureID_Invalid（即 0xFFFFFFFFFFFFFFFF），
    //             渲染时 vkCmdBindDescriptorSets 读取该无效句柄引发访问冲突崩溃。
    //   过程：设置为与 RenderDeviceVulkan::Initialize 中 require_api_version(1,3,0) 一致的值。
    init_info.ApiVersion     = VK_API_VERSION_1_3;

    init_info.Instance       = device->GetVkInstance();
    init_info.PhysicalDevice = device->GetPhysicalDevice();
    init_info.Device         = device->GetVkDevice();
    init_info.QueueFamily    = device->GetGraphicsQueueFamily();
    init_info.Queue          = device->GetGraphicsQueue();

    VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 } };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = pool_sizes;
    vkCreateDescriptorPool(device->GetVkDevice(), &pool_info, nullptr, &m_imguiDescriptorPool);

    // 创建 ImGui 使用的 Sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    vkCreateSampler(device->GetVkDevice(), &samplerInfo, nullptr, &m_imguiSampler);

    // 初始化 ImGui Vulkan 资源管理器
    m_imguiResourceManager = std::make_unique<ImGuiVulkanResourceManager>();
    m_imguiResourceManager->Initialize(device->GetVkDevice());

    init_info.DescriptorPool = m_imguiDescriptorPool;
    init_info.PipelineCache  = VK_NULL_HANDLE;
    init_info.MinImageCount  = 2;    // Vulkan 规范要求 >= 2
    init_info.ImageCount     = vkDevice->GetSwapChain()->GetBufferCount();    // 与交换链缓冲数对齐
    init_info.UseDynamicRendering               = false;
    init_info.Allocator           = nullptr;
    init_info.CheckVkResultFn                   = nullptr;

    // RenderPass / MSAA 设置（使用交换链的 RenderPass，禁用多重采样）
    init_info.PipelineInfoMain.RenderPass  = device->GetOverlayRenderPass();
    init_info.PipelineInfoMain.Subpass     = 0;
    // [修复] MSAASamples 必须显式设置为 VK_SAMPLE_COUNT_1_BIT（= 1）。
    //   若保持 0（零值初始化默认），ImGui 内部在创建 pipeline 时
    //   vkCreateGraphicsPipelines 会收到无效的采样数，driver 可能返回错误。
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    // 副视口使用相同的管线配置（当前已禁用多视口，此字段实际未使用）
    init_info.PipelineInfoForViewports = init_info.PipelineInfoMain;

    if (!ImGui_ImplVulkan_Init(&init_info)) {
        return -1;
    }

    // -----------------------------------------------------------------------
    vkDevice->SetOverlayRenderCallback([this](VkCommandBuffer cmd) {
        // [修复] 必须在渲染前设置正确的 Context
        ImGui::SetCurrentContext((ImGuiContext*)this->GetImGuiContext());
        
        if (ImGui::GetCurrentContext() && ImGui::GetDrawData()) {
            // 这里可以添加对特定纹理的布局转换逻辑（如果需要）
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
        }
    });

    return 0;

}
void Editor::OnUpdate(Timestep ts) {
    Application::OnUpdate(ts);

    // 更新窗口标题
    static std::string lastTitle = "";
    std::string projectName = m_projectSettingsWindow.GetSettings().productName;
    if (m_IsProjectDirty) projectName += "*";

    std::string sceneName = "None";
    bool sceneDirty = false;
    if (auto sceneManager = Engine::Get().GetSceneManager()) {
        if (auto scene = sceneManager->GetCurrentScene()) {
            sceneName = scene->GetName();
            sceneDirty = scene->IsDirty();
        }
    }
    if (sceneDirty) sceneName += "*";

    std::string title = projectName + " - " + sceneName + " - Prisma Engine (Vulkan)";
    if (title != lastTitle) {
        Engine::Get().GetWindow().SetTitle(title);
        lastTitle = title;
    }
}

void* Editor::GetImGuiContext() {
    return (void*)ImGui::GetCurrentContext();
}

void Editor::OnImGuiRender() {
    // 1. ImGui 帧开始
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // 2. 渲染所有 Layer 的 UI
    Application::OnImGuiRender();

    // 渲染 Editor 自己的内置窗口
    if (m_showProjectSettings) {
        m_projectSettingsWindow.Draw(&m_showProjectSettings);
    }

    // 3. ImGui 帧结束
    ImGui::Render();
    // 注意：不再在这里调用 ImGui_ImplVulkan_RenderDrawData
    // 而是由 RenderDeviceVulkan::EndFrame 在正确的 RenderPass 中调用
}

void Editor::OnRender() {
    // 这里放置场景提交逻辑 (由 Engine 循环调用)
    Application::OnRender();
    OnImGuiRender();
}

// -----------------------------------------------------------------------
// [改动] OnShutdown
//
// 目的：
//   修复程序退出时偶发的 0xc0000005 崩溃和 Vulkan 验证层严重错误。
//
// 过程：
//   1. 显式清除 OverlayRenderCallback 为 nullptr，断开引擎渲染循环与
//      即将销毁的编辑器 DLL 逻辑（捕捉了 this 的 Lambda）之间的联系。
//   2. 调整销毁顺序：必须先调用 ImGui_ImplVulkan_Shutdown，
//      再手动销毁我们自己创建的 DescriptorPool 和 Sampler。
//      因为 ImGui 后端内部可能在销毁过程中仍持有这些资源的句柄。
// -----------------------------------------------------------------------
void Editor::OnShutdown() {
    LOG_INFO("Editor", "正在关闭编辑器...");

    // 1. 立即停止渲染回调，防止后续帧进入
    if (auto renderSystem = Engine::Get().GetRenderSystem()) {
        if (renderSystem->GetDevice()) {
            auto* vkDevice = static_cast<Prisma::Graphic::Vulkan::RenderDeviceVulkan*>(renderSystem->GetDevice());
            vkDevice->SetOverlayRenderCallback(nullptr);
            vkDevice->WaitForIdle();
        }
    }

    // 2. 首先关闭 ImGui 后端（它可能正在引用下面的 Pool 或 Sampler）
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    // 3. 然后再安全地销毁我们自己管理的资源
    if (auto renderSystem = Engine::Get().GetRenderSystem()) {
        if (renderSystem->GetDevice()) {
            auto* vkDevice = static_cast<Prisma::Graphic::Vulkan::RenderDeviceVulkan*>(renderSystem->GetDevice());
            VkDevice vkDev = vkDevice->GetVkDevice();
            
            if (m_imguiResourceManager) {
                m_imguiResourceManager->Shutdown();
                m_imguiResourceManager.reset();
            }

            if (m_imguiDescriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(vkDev, m_imguiDescriptorPool, nullptr);
                m_imguiDescriptorPool = VK_NULL_HANDLE;
            }
            if (m_imguiSampler != VK_NULL_HANDLE) {
                vkDestroySampler(vkDev, m_imguiSampler, nullptr);
                m_imguiSampler = VK_NULL_HANDLE;
            }
        }
    }
}

}  // namespace Prisma

// ============================================================================
// Factory
// ============================================================================
extern "C" EDITOR_API Prisma::Application* CreateApplication() {
    return new Prisma::Editor();
}
