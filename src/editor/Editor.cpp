#include "Editor.h"
#include "EditorLayer.h"
#include "../engine/Engine.h"
#include "../engine/Platform.h"
#include "../engine/graphic/RenderSystem.h"
#include "graphic/ImGuiVulkanResourceManager.h"

// ImGui
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <SDL3/SDL_vulkan.h>
#include <imgui_impl_vulkan.h>

// 访问具体的 Vulkan 设备类型，以注册 ImGui 渲染回调
#include "graphic/adapters/vulkan/RenderDeviceVulkan.h"
#include "graphic/adapters/vulkan/VulkanResources.h"

// vk-bootstrap for Editor side Vulkan init
#include <VkBootstrap.h>

namespace Prisma {

Editor* Editor::s_Instance = nullptr;

Editor::Editor() {
    s_Instance = this;
}

Editor::~Editor() {
    Shutdown();
    s_Instance = nullptr;
}

int Editor::Initialize() {
    // 1. 初始化 SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        LOG_FATAL("Editor", "Failed to initialize SDL: {0}", SDL_GetError());
        return -1;
    }

    // 2. 创建主窗口 (必须带 SDL_WINDOW_VULKAN)
    m_Window = SDL_CreateWindow("Prisma Editor", 1600, 900, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!m_Window) {
        LOG_FATAL("Editor", "Failed to create SDL window: {0}", SDL_GetError());
        return -1;
    }

    // 3. 编辑器侧初始化 Vulkan 环境 (作为 Master)
    vkb::InstanceBuilder inst_builder;
    auto inst_ret = inst_builder.set_app_name("Prisma Editor")
                        .request_validation_layers(true)
                        .use_default_debug_messenger()
                        .require_api_version(1, 3, 0)
                        .build();
    if (!inst_ret) return -1;
    vkb::Instance vkb_inst = inst_ret.value();
    
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(m_Window, vkb_inst.instance, nullptr, &surface)) return -1;

    vkb::PhysicalDeviceSelector selector{vkb_inst};
    auto phys_ret = selector.set_surface(surface)
                        .set_minimum_version(1, 3)
                        .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                        .select();
    if (!phys_ret) return -1;
    vkb::PhysicalDevice vkb_phys = phys_ret.value();

    vkb::DeviceBuilder device_builder{vkb_phys};
    auto dev_ret = device_builder.build();
    if (!dev_ret) return -1;
    vkb::Device vkb_device = dev_ret.value();

    VkQueue graphicsQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    uint32_t graphicsQueueFamily = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    // [架构调整] VMA 转移到引擎侧创建，编辑器侧不再包含 VMA 实现宏

    // 4. 初始化引擎 (Headless 模式，由编辑器注入资源)
    EngineSpecification engineSpec;
    engineSpec.Name = "Prisma Engine Slave";
    engineSpec.Headless = true; 
    
    m_Engine = std::make_unique<Engine>(engineSpec);
    if (m_Engine->Initialize() != 0) {
        LOG_FATAL("Editor", "Failed to initialize Engine core!");
        return -1;
    }

    // [核心重构] 将编辑器创建好的 Vulkan 句柄注入引擎
    auto renderSystem = m_Engine->GetRenderSystem();
    auto device = renderSystem->GetDevice();
    auto* vkDevice = static_cast<Graphic::Vulkan::RenderDeviceVulkan*>(device);
    
    Graphic::IRenderDevice::ExternalVulkanInitInfo extInfo;
    extInfo.instance = vkb_inst.instance;
    extInfo.physicalDevice = vkb_phys.physical_device;
    extInfo.device = vkb_device.device;
    extInfo.graphicsQueue = graphicsQueue;
    extInfo.graphicsQueueFamily = graphicsQueueFamily;
    extInfo.allocator = VK_NULL_HANDLE; // 引擎会自己创建 VMA
    extInfo.windowHandle = m_Window;

    // 引擎设备现在完全通过外部句柄初始化其子系统
    vkDevice->InitializeExternalVulkan(extInfo);

    // 5. 初始化 ImGui 上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    // 6. 初始化 ImGui 后端 (SDL3 + Vulkan)
    ImGui_ImplSDL3_InitForVulkan(m_Window);

    ImGui_ImplVulkan_InitInfo imgui_init = {};
    imgui_init.ApiVersion = VK_API_VERSION_1_3;
    imgui_init.Instance = vkb_inst.instance;
    imgui_init.PhysicalDevice = vkb_phys.physical_device;
    imgui_init.Device = vkb_device.device;
    imgui_init.QueueFamily = graphicsQueueFamily;
    imgui_init.Queue = graphicsQueue;

    // 创建 ImGui 特有的 DescriptorPool
    VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 } };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = pool_sizes;
    vkCreateDescriptorPool(vkb_device.device, &pool_info, nullptr, &m_imguiDescriptorPool);

    // 创建采样器
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    vkCreateSampler(vkb_device.device, &samplerInfo, nullptr, &m_imguiSampler);

    imgui_init.DescriptorPool = m_imguiDescriptorPool;
    imgui_init.MinImageCount = 2;
    imgui_init.ImageCount = 3; 
    
    // [修复] 最新 ImGui API 要求
    imgui_init.PipelineInfoMain.RenderPass = vkDevice->GetOverlayRenderPass();
    imgui_init.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    if (!ImGui_ImplVulkan_Init(&imgui_init)) {
        LOG_FATAL("Editor", "Failed to initialize ImGui Vulkan backend!");
        return -1;
    }

    // 7. 初始化编辑器内部资源管理器
    m_imguiResourceManager = std::make_unique<ImGuiVulkanResourceManager>();
    m_imguiResourceManager->Initialize(vkb_device.device);

    // 8. 注册引擎渲染回调
    vkDevice->SetOverlayRenderCallback([this](VkCommandBuffer cmd) {
        ImGui::SetCurrentContext(ImGui::GetCurrentContext());
        if (ImGui::GetDrawData()) {
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
        }
    });

    // 9. 初始化编辑器层
    auto editorLayer = std::make_unique<EditorLayer>();
    m_Layers.push_back(std::move(editorLayer));

    m_Running = true;
    return 0;
}

void Editor::Run() {
    double lastFrameTime = Platform::GetTimeSeconds();

    while (m_Running) {
        double time = Platform::GetTimeSeconds();
        float deltaTime = static_cast<float>(time - lastFrameTime);
        lastFrameTime = time;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            OnEvent(event);
        }

        if (!m_Running) break;

        OnUpdate(Timestep(std::min(deltaTime, 0.1f)));

        m_Engine->BeginFrame(); 
        
        for (auto& layer : m_Layers) {
            layer->OnRender();
        }

        OnImGuiRender();

        m_Engine->EndFrame();
        m_Engine->Present();
    }
}

void Editor::OnEvent(SDL_Event& event) {
    if (event.type == SDL_EVENT_QUIT) {
        m_Running = false;
    }
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_Window)) {
        m_Running = false;
    }
}

void Editor::OnUpdate(Timestep ts) {
    m_Engine->Update(ts);
    for (auto& layer : m_Layers) {
        layer->OnUpdate(ts);
    }
}

void Editor::OnImGuiRender() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    for (auto& layer : m_Layers) {
        layer->OnImGuiRender();
    }
    
    if (m_showProjectSettings) {
        m_projectSettingsWindow.Draw(&m_showProjectSettings);
    }

    ImGui::Render();
}

void Editor::Shutdown() {
    if (!m_Running && !m_Window) return;

    LOG_INFO("Editor", "Shutting down Editor...");

    m_Layers.clear();

    if (m_Engine) {
        auto renderSystem = m_Engine->GetRenderSystem();
        if (renderSystem && renderSystem->GetDevice()) {
            auto vkDevice = static_cast<Graphic::Vulkan::RenderDeviceVulkan*>(renderSystem->GetDevice());
            vkDevice->WaitForIdle();
            vkDevice->SetOverlayRenderCallback(nullptr);

            if (m_imguiResourceManager) {
                m_imguiResourceManager->Shutdown();
                m_imguiResourceManager.reset();
            }

            if (m_imguiDescriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(vkDevice->GetVkDevice(), m_imguiDescriptorPool, nullptr);
            }
            if (m_imguiSampler != VK_NULL_HANDLE) {
                vkDestroySampler(vkDevice->GetVkDevice(), m_imguiSampler, nullptr);
            }

            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
        }

        m_Engine->Shutdown();
        m_Engine.reset();
    }

    if (m_Window) {
        SDL_DestroyWindow(m_Window);
        m_Window = nullptr;
    }

    SDL_Quit();
    m_Running = false;
}

} // namespace Prisma
