#pragma once

#include "../engine/core/Layer.h"
#include "../engine/Application.h"
#include "../engine/Engine.h"
#include "../engine/graphic/RenderSystem.h"
#include "../engine/SceneManager.h"
#include "../engine/Scene.h"
#include "../engine/GameObject.h"
#include "../engine/Transform.h"
#include "../engine/Camera.h"
#include "../engine/PhysicsSystem.h"
#include <vulkan/vulkan.h>
#include <filesystem>

namespace Prisma {

namespace Graphic::Vulkan {
    class ViewportRenderPass;
}

class EDITOR_API EditorLayer : public Layer {
public:
    EditorLayer();
    virtual ~EditorLayer() = default;

    void OnUpdate(Timestep ts) override;
    void OnRender() override;
    void OnImGuiRender() override;
    void OnEvent(Event& event) override;

private:
    std::shared_ptr<GameObject> m_editorCameraObject;
    std::shared_ptr<Graphic::Camera> m_editorCamera;
    std::shared_ptr<GameObject> m_selectedEntity = nullptr;
    
    float m_cameraSpeed = 5.0f;
    float m_cameraSensitivity = 0.1f;
    
    bool m_showDemoWindow = false;
    std::filesystem::path m_currentAssetDirectory = "assets";

    bool m_viewportFocused = false;
    bool m_viewportHovered = false;

    // -----------------------------------------------------------------------
    // [改动] m_viewportReadyFrames
    //
    // 目的：
    //   解决视口纹理 Resize 后的初次采样导致 VK_IMAGE_LAYOUT_UNDEFINED 验证错误。
    // -----------------------------------------------------------------------
    uint32_t m_viewportReadyFrames = 0;

    struct {
        float x = 0.0f;
        float y = 0.0f;
    } m_viewportSize;

    std::shared_ptr<Graphic::ITexture> m_viewportTexture = nullptr;
    std::shared_ptr<Graphic::ITexture> m_viewportDepthTexture = nullptr;
    VkDescriptorSet m_viewportDescriptorSet = VK_NULL_HANDLE;

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

}  // namespace Prisma
