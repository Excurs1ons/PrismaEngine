#pragma once

#include "../core/Editor.h"
#include "app/Application.h"
#include "app/Engine.h"
#include "core/Layer.h"
#include "graphic/RenderSystem.h"
#include "physics/PhysicsComponents.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "transform/Camera.h"
#include <SDL3/SDL.h>
#include <filesystem>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <vulkan/vulkan.h>

// ImGuizmo - 3D 变换操作器
#include "ImGuizmo.h"

// Vulkan 后端支持
#include "../graphic/ViewportRenderPass.h"
#include "graphic/adapters/vulkan/RenderDeviceVulkan.h"
#include "graphic/adapters/vulkan/VulkanResources.h"

namespace Prisma {

class EditorLayer : public Layer {
public:
    EditorLayer();

    void OnUpdate(Timestep ts) override;

    void OnRender();

    void OnImGuiRender() override;

    void OnEvent(Event& event) override;

private:
    bool m_showDemoWindow  = false;
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

    struct {
        float x = 0.0f;
        float y = 0.0f;
    } m_viewportSize;

    Node m_selectedEntity;
    std::filesystem::path m_currentAssetDirectory = "assets";

    Node m_editorCameraNode;
    std::shared_ptr<Graphic::Camera> m_editorCamera  = nullptr;
    float m_cameraSpeed                              = 5.0f;
    float m_cameraSensitivity                        = 0.1f;

    std::shared_ptr<Graphic::ITexture> m_viewportTexture      = nullptr;
    std::shared_ptr<Graphic::ITexture> m_viewportDepthTexture = nullptr;
    VkDescriptorSet m_viewportDescriptorSet                   = VK_NULL_HANDLE;
    std::shared_ptr<Graphic::Vulkan::ViewportRenderPass> m_viewportRenderPass;

    // -----------------------------------------------------------------------
    // [改动] m_textureDeletionQueue
    //
    // 目的：
    //   修复 Resize 期间 GPU 访问已销毁纹理导致的验证错误。
    //
    // 过程：
    //   当视口大小改变时，旧纹理不立即销毁，而是放入此队列。
    //   每帧清理存放时间超过 3 帧的资源，确保 GPU 已处理完相关指令。
    // -----------------------------------------------------------------------
    struct DeferredTexture {
        std::shared_ptr<Graphic::ITexture> texture;
        uint32_t framesLeft;
    };
    std::vector<DeferredTexture> m_textureDeletionQueue;

    // ImGuizmo 变换操作器状态
    ImGuizmo::OPERATION m_gizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE m_gizmoMode           = ImGuizmo::LOCAL;
    bool m_gizmoSnap                     = false;
    float m_gizmoSnapTranslation         = 0.5f;
    float m_gizmoSnapRotation            = 45.0f;
    float m_gizmoSnapScale               = 0.5f;

    // 当前场景文件路径（用于保存/另存为）
    std::string m_sceneFilePath;

    // Play/Stop 游戏模式
    bool m_playing = false;

    // FPS / FrameTime 统计
    float m_fps = 0.0f;
    float m_frameTime = 0.0f;
};

}  // namespace Prisma
