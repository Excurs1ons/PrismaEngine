#pragma once

#include "../windows/ProjectSettingsWindow.h"
#include "Export.h"
#include "WebUIEditor.h"
#include "app/Application.h"
#include "core/ManagerBase.h"
#include "core/Singleton.h"
#include "logger/Logger.h"
#include "platform/Platform.h"

#if defined(PRISMA_ENABLE_MCP)
#include "mcp/MCPSubSystem.h"
#include "mcp/tools/SceneTools.h"
#include "mcp/tools/ECSTools.h"
#include "mcp/tools/EngineTools.h"
#include "mcp/transport/TransportTCP.h"
#endif

// 显式包含 SDL3
#include <SDL3/SDL.h>
#include <memory>
#include <vulkan/vulkan.h>

namespace Prisma::Graphic::Vulkan {
class VulkanTexture;
}

namespace Prisma {

class ImGuiVulkanResourceManager;

class EDITOR_API Editor : public Application {
public:
    Editor();
    ~Editor() override;

    static Editor& Get() { return static_cast<Editor&>(Application::Get()); }

    // 被动初始化接口，仅负责应用层自身的逻辑
    int OnInitialize() override;
    void OnShutdown() override;

    void OnUpdate(Timestep ts) override;
    void OnRender() override;
    void OnImGuiRender() override;

    int OnImGuiInitialize() override;

    void* GetImGuiContext() override;

    void OpenProjectSettings() { m_showProjectSettings = true; }

    bool IsProjectDirty() const { return m_IsProjectDirty; }
    void SetProjectDirty(bool dirty) { m_IsProjectDirty = dirty; }

    // 获取 ImGui DescriptorPool 和 Sampler
    VkDescriptorPool GetImGuiDescriptorPool() const { return m_imguiDescriptorPool; }
    VkSampler GetImGuiSampler() const { return m_imguiSampler; }

    // 获取 ImGui 资源管理器
    ImGuiVulkanResourceManager& GetImGuiResourceManager() { return *m_imguiResourceManager; }

private:
    // Editor Windows
    ProjectSettingsWindow m_projectSettingsWindow;
    bool m_showProjectSettings = false;

    VkDescriptorPool m_imguiDescriptorPool = VK_NULL_HANDLE;
    VkSampler m_imguiSampler               = VK_NULL_HANDLE;

    // ImGui 资源管理器
    std::unique_ptr<ImGuiVulkanResourceManager> m_imguiResourceManager;
    
    // WebUI 编辑器
    std::unique_ptr<WebUIEditor> m_webUIEditor;

#if defined(PRISMA_ENABLE_MCP)
    std::unique_ptr<MCP::MCPSubSystem> m_MCP;
#endif

    bool m_IsProjectDirty = false;
};

}  // namespace Prisma
