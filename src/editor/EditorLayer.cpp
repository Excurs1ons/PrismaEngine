#include "EditorLayer.h"
#include "graphic/ImGuiVulkanResourceManager.h"
#include "graphic/ViewportRenderPass.h"
#include "Editor.h"

// ImGui
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

// 访问具体的 Vulkan 类型
#include "graphic/adapters/vulkan/VulkanResources.h"
#include "graphic/adapters/vulkan/RenderDeviceVulkan.h"

namespace Prisma {

EditorLayer::EditorLayer() : Layer("EditorLayer") {
    m_editorCameraObject = std::make_shared<GameObject>("Editor Camera");
    m_editorCamera       = m_editorCameraObject->AddComponent<Graphic::Camera>();
    m_editorCamera->SetPerspectiveProjection(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
    m_editorCameraObject->GetTransform()->SetPosition({0, 2, 5});
}

void EditorLayer::OnUpdate(Timestep ts) {
    // 提前告知引擎跳过本帧默认 Pass
    auto vkDevice = static_cast<Graphic::Vulkan::RenderDeviceVulkan*>(Engine::Get().GetRenderSystem()->GetDevice());
    if (vkDevice) {
        vkDevice->SetSkipSwapChainRenderPass(true);
    }

    if (!m_viewportHovered || !ImGui::IsMouseDown(ImGuiMouseButton_Right))
        return;

    float dt       = ts.GetSeconds();
    auto transform = m_editorCameraObject->GetTransform();
    Vector3 pos    = transform->GetPosition();

    const uint8_t* state = (const uint8_t*)SDL_GetKeyboardState(NULL);

    Vector3 forward = m_editorCamera->GetForward();
    Vector3 right   = m_editorCamera->GetRight();
    Vector3 up      = m_editorCamera->GetUp();

    if (state[SDL_SCANCODE_W]) pos += forward * m_cameraSpeed * dt;
    if (state[SDL_SCANCODE_S]) pos -= forward * m_cameraSpeed * dt;
    if (state[SDL_SCANCODE_A]) pos -= right * m_cameraSpeed * dt;
    if (state[SDL_SCANCODE_D]) pos += right * m_cameraSpeed * dt;
    if (state[SDL_SCANCODE_E]) pos += up * m_cameraSpeed * dt;
    if (state[SDL_SCANCODE_Q]) pos -= up * m_cameraSpeed * dt;

    transform->SetPosition(pos);

    // Rotation
    ImGuiIO& io = ImGui::GetIO();
    if (io.MouseDelta.x != 0 || io.MouseDelta.y != 0) {
        float yaw   = -io.MouseDelta.x * m_cameraSensitivity;
        float pitch = -io.MouseDelta.y * m_cameraSensitivity;
        m_editorCamera->Rotate(pitch, yaw, 0.0f);
    }
}

void EditorLayer::OnRender() {
    auto renderSystem = Engine::Get().GetRenderSystem();
    auto sceneManager = Engine::Get().GetSceneManager();

    if (!sceneManager) return;
    auto* scene = sceneManager->GetCurrentScene();
    if (!scene) return;

    Graphic::ICamera* camera = m_editorCamera.get();
    if (!camera) {
        auto mainCamera = scene->GetMainCamera();
        if (mainCamera) camera = mainCamera.get();
    }
    if (!camera) return;

    auto vkDevice = static_cast<Graphic::Vulkan::RenderDeviceVulkan*>(renderSystem->GetDevice());
    if (!vkDevice) return;

    VkCommandBuffer cmd = vkDevice->GetCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) return;

    // [手动接管离屏 RenderPass]
    if (m_viewportRenderPass && m_viewportRenderPass->IsInitialized()) {
        m_viewportRenderPass->Begin(cmd);
        renderSystem->RenderScene(scene, camera, m_viewportTexture.get());
        m_viewportRenderPass->End(cmd);
    }
}

void EditorLayer::OnImGuiRender() {
    static bool dockspaceOpen = true;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::Begin("Prisma Editor DockSpace", &dockspaceOpen, window_flags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Alt+F4")) Editor::Get().Shutdown();
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    ImGui::End();

    // Viewport Panel
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin("Viewport");

    m_viewportFocused = ImGui::IsWindowFocused();
    m_viewportHovered = ImGui::IsWindowHovered();

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    if (std::abs(m_viewportSize.x - viewportPanelSize.x) > 0.1f || 
        std::abs(m_viewportSize.y - viewportPanelSize.y) > 0.1f) {
        
        m_viewportSize = {viewportPanelSize.x, viewportPanelSize.y};

        if (m_viewportSize.x > 1.0f && m_viewportSize.y > 1.0f) {
            auto renderSystem = Engine::Get().GetRenderSystem();
            auto resourceManager = renderSystem->GetRenderResourceManager();
            auto vkDevice = static_cast<Graphic::Vulkan::RenderDeviceVulkan*>(renderSystem->GetDevice());
            
            // 延迟清理旧资源
            if (m_viewportTexture) m_deferredDeletionQueue.push_back({m_viewportTexture, 3});
            if (m_viewportDepthTexture) m_deferredDeletionQueue.push_back({m_viewportDepthTexture, 3});
            if (m_viewportRenderPass) m_deferredDeletionQueue.push_back({m_viewportRenderPass, 3});

            // 1. 创建离屏颜色纹理
            Graphic::TextureDesc colorDesc;
            colorDesc.width = (uint32_t)m_viewportSize.x;
            colorDesc.height = (uint32_t)m_viewportSize.y;
            colorDesc.format = Graphic::TextureFormat::RGBA8_UNorm;
            colorDesc.allowRenderTarget = true;
            colorDesc.allowShaderResource = true;
            m_viewportTexture = resourceManager->CreateTexture(colorDesc);

            // 2. 创建离屏深度纹理
            Graphic::TextureDesc depthDesc = colorDesc;
            depthDesc.format = Graphic::TextureFormat::D32_Float;
            depthDesc.allowDepthStencil = true;
            depthDesc.allowRenderTarget = false;
            depthDesc.allowShaderResource = false;
            m_viewportDepthTexture = resourceManager->CreateTexture(depthDesc);

            auto vkTexture = dynamic_cast<Graphic::Vulkan::VulkanTexture*>(m_viewportTexture.get());
            auto vkDepth = dynamic_cast<Graphic::Vulkan::VulkanTexture*>(m_viewportDepthTexture.get());

            if (vkTexture && vkDepth && vkDevice) {
                vkTexture->SetDebugName("Viewport Color");
                
                // 3. 创建 Viewport RenderPass (针对离屏纹理)
                m_viewportRenderPass = std::make_shared<Graphic::Vulkan::ViewportRenderPass>();
                m_viewportRenderPass->Initialize(
                    vkDevice->GetVkDevice(), vkTexture->GetVkImageView(),
                    vkDepth->GetVkImageView(), (uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y
                );
                
                // 4. 创建 ImGui DescriptorSet (GPU 零拷贝)
                auto& editor = Editor::Get();
                m_viewportDescriptorSet = editor.GetImGuiResourceManager().GetDescriptorSet(
                    vkTexture, editor.GetImGuiDescriptorPool(), editor.GetImGuiSampler());
            }
            m_viewportReadyFrames = 0;
        }
    }

    // [零拷贝 GPU 显示]
    if (m_viewportTexture && m_viewportDescriptorSet && m_viewportReadyFrames >= 2) {
        ImGui::Image((ImTextureID)m_viewportDescriptorSet, ImVec2{m_viewportSize.x, m_viewportSize.y});
    } else {
        ImGui::Text("Viewport Syncing...");
    }

    ImGui::End();
    ImGui::PopStyleVar();

    if (m_viewportTexture) m_viewportReadyFrames++;

    // 清理延迟队列
    for (auto it = m_deferredDeletionQueue.begin(); it != m_deferredDeletionQueue.end();) {
        if (it->framesLeft == 0) it = m_deferredDeletionQueue.erase(it);
        else { it->framesLeft--; ++it; }
    }

    ImGui::Begin("Properties");
    if (m_selectedEntity) {
        ImGui::Text("Entity: %s", m_selectedEntity->name.c_str());
    } else {
        ImGui::Text("Select an entity to view properties");
    }
    ImGui::End();
}

void EditorLayer::OnEvent(Event& event) {
    // 逻辑分发
}

} // namespace Prisma
