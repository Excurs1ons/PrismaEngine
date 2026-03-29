#pragma once

#include "../engine/core/Layer.h"
#include "../engine/Application.h"
#include "../engine/Engine.h"
#include "../engine/graphic/RenderSystem.h"
#include "../engine/SceneManager.h"
#include "../engine/Scene.h"
#include "../engine/Camera.h"
#include "../engine/physics/PhysicsComponents.h"
#include <filesystem>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include "Editor.h"

// Vulkan 后端支持
#include "../engine/graphic/adapters/vulkan/RenderDeviceVulkan.h"
#include "../engine/graphic/adapters/vulkan/VulkanResources.h"
#include "graphic/ViewportRenderPass.h"

namespace Prisma {

class EditorLayer : public Layer {
public:
    EditorLayer();

    void OnUpdate(Timestep ts) override;

    void OnRender();

    void OnImGuiRender() override;

    void OnEvent(Event& event) override;

private:
    bool m_showDemoWindow = false;
    bool m_viewportFocused = false;
    bool m_viewportHovered = false;

    // -----------------------------------------------------------------------
    // [改动] m_viewportReadyFrames
    //
    // 目的：
    //   解决视口纹理 Resize 后的初次采样导致 VK_IMAGE_LAYOUT_UNDEFINED 验证错误。
    //
    // 过程：
    //   通过帧计数器延迟视口纹理在 ImGui 中的显示。确保 GPU 有足够的时间
    //   执行初次布局转换并在命令队列中完成提交。
    // -----------------------------------------------------------------------
    uint32_t m_viewportReadyFrames = 0;

    struct { float x = 0.0f; float y = 0.0f; } m_viewportSize;

    std::shared_ptr<GameObject> m_selectedEntity = nullptr;
    std::filesystem::path m_currentAssetDirectory = "assets";

    std::shared_ptr<GameObject> m_editorCameraObject = nullptr;
    std::shared_ptr<Graphic::Camera> m_editorCamera = nullptr;
    float m_cameraSpeed = 5.0f;
    float m_cameraSensitivity = 0.1f;

    std::shared_ptr<Graphic::ITexture> m_viewportTexture = nullptr;
    std::shared_ptr<Graphic::ITexture> m_viewportDepthTexture = nullptr;
    
    // [改动] 使用 SDL_Texture 替代 VkDescriptorSet 
    // 目的：支持在非 Vulkan 的 ImGui 后端（SDL_Renderer）中显示视口内容。
    SDL_Texture* m_viewportSDLTexture = nullptr;

    std::shared_ptr<Graphic::Vulkan::ViewportRenderPass> m_viewportRenderPass;

    // -----------------------------------------------------------------------
    // [改动] m_deferredDeletionQueue
    //
    // 目的：
    //   修复资源（纹理、Pass、FB）在 GPU 尚在使用时被析构导致的崩溃。
    // -----------------------------------------------------------------------
    struct DeferredResource {
        std::shared_ptr<void> resource;
        uint32_t framesLeft;
    };
    std::vector<DeferredResource> m_deferredDeletionQueue;
};

} // namespace Prisma
