#pragma once

#include "Export.h"
#include "../engine/Engine.h"
#include "../engine/Logger.h"
#include "../engine/core/Timestep.h"
#include "ProjectSettingsWindow.h"

// 显式包含 SDL3
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace Prisma {

class EditorLayer;
class ImGuiVulkanResourceManager;

/**
 * @brief 编辑器主应用程序
 * 
 * [架构重构] 编辑器现在是系统的 Master，持有并驱动 Engine。
 * UI 渲染使用 ImGui Vulkan 后端，与引擎视口共享 GPU 设备。
 */
class EDITOR_API Editor {
public:
    Editor();
    ~Editor();

    int Initialize();
    void Run();
    void Shutdown();

    static Editor& Get() { return *s_Instance; }

    // 获取引擎实例
    Engine& GetEngine() { return *m_Engine; }
    
    // 获取 ImGui 资源管理器 (用于视口纹理转换)
    ImGuiVulkanResourceManager& GetImGuiResourceManager() { return *m_imguiResourceManager; }

    void OpenProjectSettings() { m_showProjectSettings = true; }

    // 获取 ImGui 所需的 Vulkan 资源句柄
    VkDescriptorPool GetImGuiDescriptorPool() const { return m_imguiDescriptorPool; }
    VkSampler GetImGuiSampler() const { return m_imguiSampler; }

private:
    void OnUpdate(Timestep ts);
    void OnRender();
    void OnImGuiRender();

    // 事件处理
    void OnEvent(SDL_Event& event);

private:
    static Editor* s_Instance;

    std::unique_ptr<Engine> m_Engine;
    bool m_Running = false;

    // 窗口资源 (由编辑器管理)
    SDL_Window* m_Window = nullptr;

    // ImGui Vulkan 资源
    VkDescriptorPool m_imguiDescriptorPool = VK_NULL_HANDLE;
    VkSampler m_imguiSampler = VK_NULL_HANDLE;

    // 编辑器状态
    ProjectSettingsWindow m_projectSettingsWindow;
    bool m_showProjectSettings = false;
    bool m_showDemoWindow = true;

    // ImGui 资源管理器
    std::unique_ptr<ImGuiVulkanResourceManager> m_imguiResourceManager;
    
    // 编辑器层
    std::vector<std::unique_ptr<EditorLayer>> m_Layers;
};

} // namespace Prisma
