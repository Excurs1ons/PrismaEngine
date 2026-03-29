#pragma once

#include "Export.h"
#include "../engine/Engine.h"
#include "../engine/Logger.h"
#include "../engine/core/Timestep.h"
#include "ProjectSettingsWindow.h"

// 显式包含 SDL3
#include <SDL3/SDL.h>
#include <memory>
#include <vector>

namespace Prisma {

class EditorLayer;
class ImGuiVulkanResourceManager;

/**
 * @brief 编辑器主应用程序
 * 
 * [架构重构] 编辑器现在是系统的 Master，它持有 Engine 实例并驱动主循环。
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
    
    // 获取 SDL 渲染器 (用于 UI)
    SDL_Renderer* GetRenderer() { return m_Renderer; }
    
    // 获取 ImGui 资源管理器 (用于视口纹理转换)
    ImGuiVulkanResourceManager& GetImGuiResourceManager() { return *m_imguiResourceManager; }

    void OpenProjectSettings() { m_showProjectSettings = true; }

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

    // SDL 资源 (用于 UI 渲染)
    SDL_Window* m_Window = nullptr;
    SDL_Renderer* m_Renderer = nullptr; // 用于 UI

    // 编辑器状态
    ProjectSettingsWindow m_projectSettingsWindow;
    bool m_showProjectSettings = false;
    bool m_showDemoWindow = true;

    // ImGui 资源管理器
    std::unique_ptr<ImGuiVulkanResourceManager> m_imguiResourceManager;
    
    // 编辑器层 (原来的 Application 逻辑移到这里)
    std::vector<std::unique_ptr<EditorLayer>> m_Layers;
};

} // namespace Prisma
