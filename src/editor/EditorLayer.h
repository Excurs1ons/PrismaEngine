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
#include "Editor.h"

namespace Prisma {

class EditorLayer : public Layer {
public:
    EditorLayer() : Layer("EditorLayer") {
        m_editorCameraObject = std::make_shared<GameObject>("Editor Camera");
        m_editorCamera = m_editorCameraObject->AddComponent<Graphic::Camera>();
        m_editorCamera->SetPerspectiveProjection(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
        m_editorCameraObject->GetTransform()->SetPosition({0, 2, 5});
    }

    void OnUpdate(Timestep ts) override {
        if (!m_viewportHovered || !ImGui::IsMouseDown(ImGuiMouseButton_Right)) return;

        float dt = ts.GetSeconds();
        auto transform = m_editorCameraObject->GetTransform();
        Vector3 pos = transform->GetPosition();

        const uint8_t* state = (const uint8_t*)SDL_GetKeyboardState(NULL);
        
        Vector3 forward = m_editorCamera->GetForward();
        Vector3 right = m_editorCamera->GetRight();
        Vector3 up = m_editorCamera->GetUp();

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
            float yaw = -io.MouseDelta.x * m_cameraSensitivity;
            float pitch = -io.MouseDelta.y * m_cameraSensitivity;
            m_editorCamera->Rotate(pitch, yaw, 0.0f);
        }
    }

    void OnRender() {
        auto renderSystem = Engine::Get().GetRenderSystem();

        // 渲染场景
        auto sceneManager = Engine::Get().GetSceneManager();

        if (sceneManager) {
            auto* scene = sceneManager->GetCurrentScene();
            if (scene) {
                // Use editor camera if available
                if (m_editorCamera) {
                    renderSystem->RenderScene(scene, m_editorCamera.get());
                } else {
                    auto camera = scene->GetMainCamera();
                    if (camera) {
                        renderSystem->RenderScene(scene, camera.get());
                    }
                }
            }
        }
    }

    void OnImGuiRender() override {
        static bool dockspaceOpen = true;
        static bool opt_fullscreen = true;
        static bool opt_padding = false;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen) {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        } else {
            dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
        }

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        if (!opt_padding)
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("Prisma Editor DockSpace", &dockspaceOpen, window_flags);

        if (!opt_padding)
            ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // Submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                    if (auto sceneManager = Engine::Get().GetSceneManager()) {
                        sceneManager->CreateNewScene();
                        m_selectedEntity = nullptr;
                    }
                }
                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    Application::Get().Close();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Project Settings")) {
                    static_cast<Editor&>(Application::Get()).OpenProjectSettings();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("ImGui Demo Window", nullptr, &m_showDemoWindow)) {}
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();

        // Viewport Panel
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Viewport");
        
        m_viewportFocused = ImGui::IsWindowFocused();
        m_viewportHovered = ImGui::IsWindowHovered();
        
        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        if (m_viewportSize.x != viewportPanelSize.x || m_viewportSize.y != viewportPanelSize.y) {
            m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };
            
            // Recreate Framebuffer texture when viewport resizes
            if (m_viewportSize.x > 0 && m_viewportSize.y > 0) {
                if (auto renderSystem = Engine::Get().GetRenderSystem()) {
                    if (auto resourceManager = renderSystem->GetRenderResourceManager()) {
                        Graphic::TextureDesc desc;
                        desc.width = (uint32_t)m_viewportSize.x;
                        desc.height = (uint32_t)m_viewportSize.y;
                        desc.format = Graphic::TextureFormat::RGBA8_UNorm;
                        desc.allowRenderTarget = true;
                        desc.allowShaderResource = true;
                        
                        m_viewportTexture = resourceManager->CreateTexture(desc);
                    }
                }
            }
        }

        // Draw Framebuffer image
        if (m_viewportTexture) {
            void* textureID = (void*)m_viewportTexture->GetDefaultSRV();
            if (textureID) {
                ImGui::Image((ImTextureID)textureID, ImVec2{ m_viewportSize.x, m_viewportSize.y });
            } else {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Viewport texture handle invalid");
            }
        } else {
            // Draw a placeholder background
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 min = ImGui::GetWindowPos();
            ImVec2 max = ImVec2(min.x + m_viewportSize.x, min.y + m_viewportSize.y);
            drawList->AddRectFilled(min, max, IM_COL32(30, 30, 30, 255));
            
            // Draw a simple grid
            float gridSize = 64.0f;
            for (float x = 0; x < m_viewportSize.x; x += gridSize)
                drawList->AddLine(ImVec2(min.x + x, min.y), ImVec2(min.x + x, max.y), IM_COL32(60, 60, 60, 255));
            for (float y = 0; y < m_viewportSize.y; y += gridSize)
                drawList->AddLine(ImVec2(min.x, min.y + y), ImVec2(max.x, min.y + y), IM_COL32(60, 60, 60, 255));

            ImGui::SetCursorPos(ImVec2(viewportPanelSize.x * 0.5f - 80, viewportPanelSize.y * 0.5f - 10));
            ImGui::Text("Render Pipeline Initializing...");
        }
        
        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Begin("Scene Hierarchy");
        if (auto sceneManager = Engine::Get().GetSceneManager()) {
            if (auto scene = sceneManager->GetCurrentScene()) {
                auto& objects = scene->GetGameObjects();
                for (auto& obj : objects) {
                    ImGuiTreeNodeFlags flags = ((m_selectedEntity == obj) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
                    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
                    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)obj.get(), flags, "%s", obj->name.c_str());
                    if (ImGui::IsItemClicked()) {
                        m_selectedEntity = obj;
                    }
                    if (opened) {
                        ImGui::TreePop();
                    }
                }

                // Right-click on blank space
                if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight)) {
                    if (ImGui::MenuItem("Create Empty GameObject")) {
                        auto newObj = std::make_shared<GameObject>("New GameObject");
                        scene->AddGameObject(newObj);
                        m_selectedEntity = newObj;
                    }
                    ImGui::EndPopup();
                }
            }
        }
        ImGui::End();

        ImGui::Begin("Properties");
        if (m_selectedEntity) {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strncpy(buffer, m_selectedEntity->name.c_str(), sizeof(buffer));
            if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
                m_selectedEntity->name = std::string(buffer);
            }

            ImGui::Separator();

            auto transform = m_selectedEntity->GetTransform();
            if (transform) {
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    Vector3 pos = transform->GetPosition();
                    if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
                        transform->SetPosition(pos);
                    }

                    Vector3 rotation = glm::degrees(glm::eulerAngles(transform->GetRotation()));
                    if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f)) {
                        transform->SetRotation(rotation);
                    }

                    Vector3 scale = transform->GetScale();
                    if (ImGui::DragFloat3("Scale", &scale.x, 0.1f)) {
                        transform->SetScale(scale);
                    }
                }
            }
            
            // Camera Component display
            auto camera = m_selectedEntity->GetComponent<Graphic::Camera>();
            if (camera) {
                if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
                    float fov = glm::degrees(camera->GetFOV());
                    if (ImGui::DragFloat("Field of View", &fov, 1.0f, 10.0f, 120.0f)) {
                        camera->SetPerspectiveProjection(glm::radians(fov), camera->GetAspectRatio(), camera->GetNearPlane(), camera->GetFarPlane());
                    }
                }
            }

            // RigidBody Component display
            auto rb = m_selectedEntity->GetComponent<RigidBodyComponent>();
            if (rb) {
                if (ImGui::CollapsingHeader("RigidBody", ImGuiTreeNodeFlags_DefaultOpen)) {
                    Vector3 vel = rb->GetVelocity();
                    if (ImGui::DragFloat3("Velocity", &vel.x, 0.1f)) {
                        rb->SetVelocity(vel);
                    }
                }
            }
            
            // Add Component button
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::Button("Add Component...", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                ImGui::OpenPopup("AddComponentPopup");
            }
            
            if (ImGui::BeginPopup("AddComponentPopup")) {
                if (ImGui::MenuItem("Camera")) {
                    if (!m_selectedEntity->GetComponent<Graphic::Camera>())
                        m_selectedEntity->AddComponent<Graphic::Camera>();
                }
                if (ImGui::MenuItem("RigidBody")) {
                    if (!m_selectedEntity->GetComponent<RigidBodyComponent>())
                        m_selectedEntity->AddComponent<RigidBodyComponent>();
                }
                ImGui::EndPopup();
            }
        } else {
            ImGui::Text("Select an entity to view properties");
        }
        ImGui::End();

        ImGui::Begin("Content Browser");
        
        // Ensure assets directory exists to prevent crash
        if (!std::filesystem::exists("assets")) {
            std::filesystem::create_directory("assets");
        }
        if (!std::filesystem::exists(m_currentAssetDirectory)) {
            m_currentAssetDirectory = "assets";
        }

        if (m_currentAssetDirectory != "assets") {
            if (ImGui::Button("<- Back")) {
                m_currentAssetDirectory = m_currentAssetDirectory.parent_path();
            }
        }
        
        static float padding = 16.0f;
        static float thumbnailSize = 128.0f;
        float cellSize = thumbnailSize + padding;

        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = (int)(panelWidth / cellSize);
        if (columnCount < 1) columnCount = 1;

        ImGui::Columns(columnCount, 0, false);

        for (auto& directoryEntry : std::filesystem::directory_iterator(m_currentAssetDirectory)) {
            const auto& path = directoryEntry.path();
            std::string filenameString = path.filename().string();

            ImGui::PushID(filenameString.c_str());
            
            if (directoryEntry.is_directory()) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.7f, 1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            }

            if (ImGui::Button(filenameString.c_str(), { thumbnailSize, thumbnailSize })) {
                if (directoryEntry.is_directory()) {
                    m_currentAssetDirectory /= path.filename();
                }
            }

            ImGui::PopStyleColor();
            
            ImGui::TextWrapped("%s", filenameString.c_str());
            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);
        ImGui::End();

        if (m_showDemoWindow) {
            ImGui::ShowDemoWindow(&m_showDemoWindow);
        }
    }

    void OnEvent(Event& event) override {
        if (event.NativeEvent) {
            ImGui_ImplSDL3_ProcessEvent((const SDL_Event*)event.NativeEvent);
        }

        // 检查 ImGui 是否想要捕获此事件。如果是，则标记事件为已处理，防止其传递给下层（如游戏世界）。
        ImGuiIO& io = ImGui::GetIO();
        event.Handled |= event.IsInCategory(EventCategoryMouse) && io.WantCaptureMouse;
        event.Handled |= event.IsInCategory(EventCategoryKeyboard) && io.WantCaptureKeyboard;
    }

private:
    bool m_showDemoWindow = false;
    bool m_viewportFocused = false;
    bool m_viewportHovered = false;
    struct { float x = 0.0f; float y = 0.0f; } m_viewportSize;

    std::shared_ptr<GameObject> m_selectedEntity = nullptr;
    std::filesystem::path m_currentAssetDirectory = "assets";

    std::shared_ptr<GameObject> m_editorCameraObject = nullptr;
    std::shared_ptr<Graphic::Camera> m_editorCamera = nullptr;
    float m_cameraSpeed = 5.0f;
    float m_cameraSensitivity = 0.1f;

    std::shared_ptr<Graphic::ITexture> m_viewportTexture = nullptr;
};

} // namespace Prisma
