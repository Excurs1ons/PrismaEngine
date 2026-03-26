#pragma once

#include "Export.h"
#include "Application.h"
#include "Logger.h"
#include "Platform.h"
#include "ProjectSettingsWindow.h"
#include "Singleton.h"
#include "ManagerBase.h"

// 显式包含 SDL3
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <memory>

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
    VkSampler m_imguiSampler = VK_NULL_HANDLE;

    // ImGui 资源管理器
    std::unique_ptr<ImGuiVulkanResourceManager> m_imguiResourceManager;
};

} // namespace Prisma
