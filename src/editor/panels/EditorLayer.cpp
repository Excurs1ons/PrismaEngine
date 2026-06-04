#include "EditorLayer.h"
#include "ProfilerPanel.h"
#include "../UIStrings.h"
#include "../graphic/ImGuiVulkanResourceManager.h"
#include "../graphic/ViewportRenderPass.h"
#include "graphic/LightComponent.h"
#include "graphic/MeshRenderer.h"
#include "graphic/adapters/vulkan/VulkanCommandBuffer.h"
#include "transform/Transform.h"
#include <glm/gtx/matrix_decompose.hpp>

Prisma::EditorLayer::EditorLayer() : Layer("EditorLayer") {
    auto* sceneMgr = Engine::Get().GetSceneManager();
    auto* scene = sceneMgr ? sceneMgr->GetCurrentScene() : nullptr;
    if (scene) {
        m_editorCameraNode = scene->CreateNode("Editor Camera");
        m_editorCamera = scene->AddComponent<Graphic::Camera>(m_editorCameraNode);
        m_editorCamera->SetPerspectiveProjection(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
        auto t = scene->GetComponent<Transform>(m_editorCameraNode);
        if (t) t->SetPosition({0, 2, 5});
    }
}

void Prisma::EditorLayer::OnUpdate(Timestep ts) {
    // FPS / FrameTime tracking
    if (ts.GetSeconds() > 0.0f) {
        m_fps = 0.9f * m_fps + 0.1f * (1.0f / ts.GetSeconds());
        m_frameTime = 0.9f * m_frameTime + 0.1f * ts.GetSeconds();
    }

    // 游戏模式运行时禁用编辑器相机控制
    if (m_playing)
        return;

    if (!m_viewportHovered || !ImGui::IsMouseDown(ImGuiMouseButton_Right))
        return;

    float dt       = ts.GetSeconds();
    auto* scene = Engine::Get().GetSceneManager()->GetCurrentScene();
    auto transform = scene ? scene->GetComponent<Transform>(m_editorCameraNode) : nullptr;
    if (!transform) return;
    Vector3 pos    = transform->GetPosition();

    const uint8_t* state = (const uint8_t*)SDL_GetKeyboardState(NULL);

    Vector3 forward = m_editorCamera->GetForward();
    Vector3 right   = m_editorCamera->GetRight();
    Vector3 up      = m_editorCamera->GetUp();

    if (state[SDL_SCANCODE_W])
        pos += forward * m_cameraSpeed * dt;
    if (state[SDL_SCANCODE_S])
        pos -= forward * m_cameraSpeed * dt;
    if (state[SDL_SCANCODE_A])
        pos -= right * m_cameraSpeed * dt;
    if (state[SDL_SCANCODE_D])
        pos += right * m_cameraSpeed * dt;
    if (state[SDL_SCANCODE_E])
        pos += up * m_cameraSpeed * dt;
    if (state[SDL_SCANCODE_Q])
        pos -= up * m_cameraSpeed * dt;

    transform->SetPosition(pos);

    // Rotation
    ImGuiIO& io = ImGui::GetIO();
    if (io.MouseDelta.x != 0 || io.MouseDelta.y != 0) {
        float yaw   = -io.MouseDelta.x * m_cameraSensitivity;
        float pitch = -io.MouseDelta.y * m_cameraSensitivity;
        m_editorCamera->Rotate(pitch, yaw, 0.0f);
    }
}

void Prisma::EditorLayer::OnRender() {
    auto renderSystem = Engine::Get().GetRenderSystem();
    auto sceneManager = Engine::Get().GetSceneManager();

    if (!sceneManager) {
        return;
    }

    auto* scene = sceneManager->GetCurrentScene();
    if (!scene) {
        return;
    }

    Graphic::ICamera* camera = m_editorCamera.get();
    if (!camera) {
        auto mainCamera = scene->GetMainCamera();
        if (mainCamera) {
            camera = mainCamera.get();
        }
    }

    if (!camera) {
        return;
    }

    // [修复] 从 ICommandBuffer* 获取原生 VkCommandBuffer 句柄
    auto vkDevice       = static_cast<Graphic::Vulkan::RenderDeviceVulkan*>(renderSystem->GetDevice());
    auto* cmdBuffer     = vkDevice->GetCurrentCommandBuffer();
    auto* vkCmdBuffer   = dynamic_cast<Graphic::Vulkan::VulkanCommandBuffer*>(cmdBuffer);
    VkCommandBuffer cmd = vkCmdBuffer ? vkCmdBuffer->GetVkCommandBuffer() : VK_NULL_HANDLE;

    if (cmd && m_viewportRenderPass) {
        m_viewportRenderPass->Begin(cmd);
        renderSystem->RenderScene(scene, camera, m_viewportTexture.get());
        m_viewportRenderPass->End(cmd);
    }
}

void Prisma::EditorLayer::OnImGuiRender() {
    ImGuizmo::BeginFrame();

    static bool dockspaceOpen                 = true;
    static bool opt_fullscreen                = true;
    static bool opt_padding                   = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    } else {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin(UI::WINDOW_DOCKSPACE, &dockspaceOpen, window_flags);

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
        if (ImGui::BeginMenu(UI::MENU_FILE)) {
            if (ImGui::BeginMenu(UI::MENU_PROJECT)) {
                if (ImGui::MenuItem(UI::ITEM_OPEN_PROJECT, "Ctrl+O")) {
                    ImGui::OpenPopup(UI::POPUP_OPEN_PROJECT);
                }
                if (ImGui::MenuItem(UI::ITEM_NEW_PROJECT, "Ctrl+N")) {
                    ImGui::OpenPopup(UI::POPUP_NEW_PROJECT);
                }
                ImGui::Separator();
                if (ImGui::MenuItem(UI::ITEM_SAVE_PROJECT, "Ctrl+S")) {
                    LOG_INFO("Editor", "项目已保存 (暂存实现)");
                    Editor::Get().SetProjectDirty(false);
                }
                if (ImGui::MenuItem(UI::ITEM_SAVE_PROJECT_AS, "Ctrl+Shift+S")) {
                    ImGui::OpenPopup(UI::POPUP_SAVE_PROJECT_AS);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(UI::MENU_SCENE)) {
                if (ImGui::MenuItem(UI::ITEM_NEW_SCENE, "Ctrl+L")) {
                    if (auto sceneManager = Engine::Get().GetSceneManager()) {
                        sceneManager->CreateNewScene();
                        m_selectedEntity = Node{};
                        LOG_INFO("Editor", "新场景已创建");
                    }
                }
                if (ImGui::MenuItem(UI::ITEM_OPEN_SCENE, "Ctrl+Shift+O")) {
                    ImGui::OpenPopup(UI::POPUP_OPEN_SCENE);
                }
                ImGui::Separator();
                if (ImGui::MenuItem(UI::ITEM_SAVE_SCENE, "Ctrl+Alt+S")) {
                    if (auto sceneManager = Engine::Get().GetSceneManager()) {
                        if (auto scene = sceneManager->GetCurrentScene()) {
                            if (!m_sceneFilePath.empty()) {
                                LOG_INFO("Editor", "正在保存场景: %s", m_sceneFilePath.c_str());
                                scene->Serialize(m_sceneFilePath);
                            } else {
                                ImGui::OpenPopup(UI::POPUP_SAVE_SCENE_AS);
                            }
                            scene->SetDirty(false);
                        }
                    }
                }
                if (ImGui::MenuItem(UI::ITEM_SAVE_SCENE_AS, "Ctrl+Alt+Shift+S")) {
                    ImGui::OpenPopup(UI::POPUP_SAVE_SCENE_AS);
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();
            if (ImGui::MenuItem(UI::ITEM_EXIT, "Alt+F4")) {
                Application::Get().Close();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(UI::MENU_EDIT)) {
            if (ImGui::MenuItem(UI::ITEM_PROJECT_SETTINGS)) {
                static_cast<Editor&>(Application::Get()).OpenProjectSettings();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(UI::MENU_VIEW)) {
            if (ImGui::MenuItem(UI::ITEM_IMGUI_DEMO, nullptr, &m_showDemoWindow)) {
            }
            ImGui::Separator();
            bool profilerOpen = ProfilerPanel::IsVisible();
            if (ImGui::MenuItem("Profiler", nullptr, &profilerOpen)) {
                ProfilerPanel::Toggle();
            }
            ImGui::EndMenu();
        }

        // Play / Stop 游戏模式
        if (!m_playing) {
            if (ImGui::MenuItem("> Play", "F5")) {
                m_playing = true;
            }
        } else {
            if (ImGui::MenuItem("[] Stop", "F5")) {
                m_playing = false;
            }
        }

        ImGui::EndMenuBar();
    }

    ImGui::End();

    // Viewport Panel
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin(UI::WINDOW_VIEWPORT);

    // Gizmo Toolbar
    {
        bool translatePressed = ImGui::Button("W##Translate");
        ImGui::SameLine();
        bool rotatePressed = ImGui::Button("E##Rotate");
        ImGui::SameLine();
        bool scalePressed = ImGui::Button("R##Scale");
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        bool worldLocalPressed = ImGui::Button(m_gizmoMode == ImGuizmo::LOCAL ? "Local" : "World");
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        if (ImGui::Checkbox("Snap", &m_gizmoSnap)) {}
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        ImGui::DragFloat("##SnapVal", &m_gizmoSnapTranslation, 0.1f, 0.01f, 100.0f, "%.2f");

        if (translatePressed || ImGui::IsKeyPressed(ImGuiKey_W)) m_gizmoOperation = ImGuizmo::TRANSLATE;
        if (rotatePressed    || ImGui::IsKeyPressed(ImGuiKey_E)) m_gizmoOperation = ImGuizmo::ROTATE;
        if (scalePressed     || ImGui::IsKeyPressed(ImGuiKey_R)) m_gizmoOperation = ImGuizmo::SCALE;
        if (worldLocalPressed) m_gizmoMode = (m_gizmoMode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;

        // 高亮当前 Gizmo 操作模式
        ImGui::SameLine();
        ImGui::TextUnformatted("|");
        ImGui::SameLine();
        ImGui::Text("%s", m_gizmoOperation == ImGuizmo::TRANSLATE ? "Translate" :
                          m_gizmoOperation == ImGuizmo::ROTATE    ? "Rotate" :
                          m_gizmoOperation == ImGuizmo::SCALE     ? "Scale" : "?");
        ImGui::SameLine();
        ImGui::TextUnformatted("|");
        ImGui::SameLine();
        ImGui::Text("%s", m_gizmoMode == ImGuizmo::LOCAL ? "Local" : "World");
    }

    m_viewportFocused = ImGui::IsWindowFocused();
    m_viewportHovered = ImGui::IsWindowHovered();

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    if (m_viewportSize.x != viewportPanelSize.x || m_viewportSize.y != viewportPanelSize.y) {
        m_viewportSize = {viewportPanelSize.x, viewportPanelSize.y};

        // Recreate Framebuffer texture when viewport resizes
        if (m_viewportSize.x > 0 && m_viewportSize.y > 0) {
            if (auto renderSystem = Engine::Get().GetRenderSystem()) {
                if (auto resourceManager = renderSystem->GetRenderResourceManager()) {
                    // [改动] 在销毁旧纹理前，显式清理 ImGui 缓存并延迟释放
                    if (m_viewportTexture) {
                        auto oldVkTexture = dynamic_cast<Graphic::Vulkan::VulkanTexture*>(m_viewportTexture.get());
                        if (oldVkTexture) {
                            Editor::Get().GetImGuiResourceManager().ReleaseTextureResources(oldVkTexture);
                        }
                        // 将旧纹理存入延迟清理队列（保留 3 帧），防止 GPU In-Flight 指令引用失效资源
                        m_textureDeletionQueue.push_back({m_viewportTexture, 3});
                    }
                    if (m_viewportDepthTexture) {
                        m_textureDeletionQueue.push_back({m_viewportDepthTexture, 3});
                    }

                    // 创建颜色纹理
                    Graphic::TextureDesc colorDesc;
                    colorDesc.width               = (uint32_t)m_viewportSize.x;
                    colorDesc.height              = (uint32_t)m_viewportSize.y;
                    colorDesc.format              = Graphic::TextureFormat::RGBA8_UNorm;
                    colorDesc.allowRenderTarget   = true;
                    colorDesc.allowShaderResource = true;

                    m_viewportTexture = resourceManager->CreateTexture(colorDesc);
                    auto vkTexture    = dynamic_cast<Graphic::Vulkan::VulkanTexture*>(m_viewportTexture.get());
                    if (vkTexture) {
                        vkTexture->SetDebugName("Viewport Color Texture");
                    }

                    // 创建深度纹理
                    Graphic::TextureDesc depthDesc;
                    depthDesc.width               = (uint32_t)m_viewportSize.x;
                    depthDesc.height              = (uint32_t)m_viewportSize.y;
                    depthDesc.format              = Graphic::TextureFormat::D32_Float;
                    depthDesc.allowDepthStencil   = true;
                    depthDesc.allowRenderTarget   = false;
                    depthDesc.allowShaderResource = false;

                    m_viewportDepthTexture = resourceManager->CreateTexture(depthDesc);
                    auto vkDepthTexture = dynamic_cast<Graphic::Vulkan::VulkanTexture*>(m_viewportDepthTexture.get());
                    if (vkDepthTexture) {
                        vkDepthTexture->SetDebugName("Viewport Depth Texture");
                    }

                    // 创建 Viewport RenderPass
                    if (vkTexture && vkDepthTexture) {
                        auto vkDevice = static_cast<Graphic::Vulkan::RenderDeviceVulkan*>(renderSystem->GetDevice());
                        if (vkDevice) {
                            m_viewportRenderPass = std::make_shared<Graphic::Vulkan::ViewportRenderPass>();
                            m_viewportRenderPass->Initialize(vkDevice->GetVkDevice(),
                                                             vkTexture->GetVkImageView(),
                                                             vkDepthTexture->GetVkImageView(),
                                                             (uint32_t)m_viewportSize.x,
                                                             (uint32_t)m_viewportSize.y);
                        }
                    }

                    // 创建 ImGui descriptor set
                    m_viewportDescriptorSet = VK_NULL_HANDLE;
                    auto& editor            = Editor::Get();
                    if (vkTexture) {
                        m_viewportDescriptorSet = editor.GetImGuiResourceManager().GetDescriptorSet(
                            vkTexture, editor.GetImGuiDescriptorPool(), editor.GetImGuiSampler());
                    }

                    // [改动] 重置计数器
                    // 目的：延迟纹理显示，规避初次采样时的布局错误。
                    m_viewportReadyFrames = 0;
                }
            }
        }
    }

    // [改动] 增加计数器判定
    // Draw Framebuffer image (只有当计数器达到足够值时才显示，确保 GPU 已完成布局转换)
    if (m_viewportTexture && m_viewportDescriptorSet && m_viewportReadyFrames >= 3) {
        // 使用 descriptor set 显示纹理
        ImGui::Image((ImTextureID)m_viewportDescriptorSet, ImVec2{m_viewportSize.x, m_viewportSize.y});
    } else {
        // ... (此处省略，代码已存在)
    }

    // -----------------------------------------------------------------------
    // ImGuizmo 变换操作器
    // -----------------------------------------------------------------------
    // Gizmo 键盘快捷键（右键未按下时触发，避免与相机 WASD 冲突）
    if (m_viewportHovered && !ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) m_gizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) m_gizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) m_gizmoOperation = ImGuizmo::SCALE;
        if (ImGui::IsKeyPressed(ImGuiKey_T))
            m_gizmoMode = (m_gizmoMode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        if (ImGui::IsKeyPressed(ImGuiKey_X)) m_gizmoSnap = !m_gizmoSnap;
    }

    // 始终绘制 ImGuizmo 网格
    if (m_editorCamera) {
        auto viewMatrix  = m_editorCamera->GetViewMatrix();
        auto projMatrix  = m_editorCamera->GetProjectionMatrix();
        Matrix4x4 gridMatrix(1.0f);
        ImGuizmo::DrawGrid(glm::value_ptr(viewMatrix), glm::value_ptr(projMatrix),
                           glm::value_ptr(gridMatrix), 100.0f);
    }

    // 选中实体时的操作器
    if (m_selectedEntity.IsValid()) {
        auto* scene = Engine::Get().GetSceneManager()->GetCurrentScene();
        if (scene) {
            auto transform = scene->GetComponent<Transform>(m_selectedEntity);
            if (transform && m_editorCamera) {
                // 视口内容区域（不含窗口装饰）
                ImVec2 viewMin = ImGui::GetWindowContentRegionMin();
                ImVec2 viewMax = ImGui::GetWindowContentRegionMax();
                ImVec2 winPos  = ImGui::GetWindowPos();
                float gizmoX   = winPos.x + viewMin.x;
                float gizmoY   = winPos.y + viewMin.y;
                float gizmoW   = viewMax.x - viewMin.x;
                float gizmoH   = viewMax.y - viewMin.y;

                auto viewMatrix = m_editorCamera->GetViewMatrix();
                auto projMatrix = m_editorCamera->GetProjectionMatrix();
                Matrix4x4 modelMatrix = transform->GetMatrix();

                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(gizmoX, gizmoY, gizmoW, gizmoH);

                float snap[3] = {};
                if (m_gizmoSnap) {
                    switch (m_gizmoOperation) {
                        case ImGuizmo::TRANSLATE:
                            snap[0] = snap[1] = snap[2] = m_gizmoSnapTranslation;
                            break;
                        case ImGuizmo::ROTATE:
                            snap[0] = snap[1] = snap[2] = m_gizmoSnapRotation;
                            break;
                        case ImGuizmo::SCALE:
                            snap[0] = snap[1] = snap[2] = m_gizmoSnapScale;
                            break;
                        default: break;
                    }
                }

                ImGuizmo::Manipulate(
                    glm::value_ptr(viewMatrix),
                    glm::value_ptr(projMatrix),
                    m_gizmoOperation,
                    m_gizmoMode,
                    glm::value_ptr(modelMatrix),
                    nullptr,
                    m_gizmoSnap ? snap : nullptr
                );

                // 将修改后的矩阵分解回 Transform 组件
                if (ImGuizmo::IsUsing()) {
                    Vector3 position, scale;
                    Quaternion rotation;
                    glm::vec3 skew;
                    glm::vec4 perspective;
                    glm::decompose(modelMatrix, scale, rotation, position, skew, perspective);

                    transform->SetPosition(position);
                    transform->SetRotation(rotation);
                    transform->SetScale(scale);
                    scene->SetDirty(true);
                }
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();

    // 在每帧结束时递增计数器
    if (m_viewportTexture) {
        m_viewportReadyFrames++;
    }

    // -----------------------------------------------------------------------
    // [改动] 清理延迟删除队列
    // -----------------------------------------------------------------------
    for (auto it = m_textureDeletionQueue.begin(); it != m_textureDeletionQueue.end();) {
        if (it->framesLeft == 0) {
            it = m_textureDeletionQueue.erase(it);
        } else {
            it->framesLeft--;
            ++it;
        }
    }

    // -----------------------------------------------------------------------
    // Scene Hierarchy
    // -----------------------------------------------------------------------
    std::string hierarchyTitle = UI::WINDOW_HIERARCHY;
    if (auto sceneManager = Engine::Get().GetSceneManager()) {
        if (auto scene = sceneManager->GetCurrentScene()) {
            hierarchyTitle += " - " + scene->GetName();
            if (scene->IsDirty())
                hierarchyTitle += "*";
        }
    }
    ImGui::Begin(hierarchyTitle.c_str());

    if (auto sceneManager = Engine::Get().GetSceneManager()) {
        if (auto scene = sceneManager->GetCurrentScene()) {
            const auto& nodes = scene->GetNodes();
            for (auto& node : nodes) {
                ImGuiTreeNodeFlags flags =
                    ((m_selectedEntity.handle == node.handle) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
                flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
                bool opened = ImGui::TreeNodeEx((void*)(uint64_t)node.handle, flags, "%s", scene->GetNodeName(node).c_str());
                if (ImGui::IsItemClicked()) {
                    m_selectedEntity = node;
                }
                // Right-click on tree node opens context menu
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    m_selectedEntity = node;
                    ImGui::OpenPopup("HierarchyContextMenu");
                }
                if (opened) {
                    ImGui::TreePop();
                }
            }

            // Right-click context menu (tree node or blank space)
            if (ImGui::BeginPopupContextWindow("HierarchyContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
                if (ImGui::MenuItem("Create Empty GameObject")) {
                    auto newNode = scene->CreateNode("New GameObject");
                    m_selectedEntity = newNode;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Duplicate Entity", "Ctrl+D", false, m_selectedEntity.IsValid())) {
                    if (m_selectedEntity.IsValid()) {
                        auto newNode = scene->CreateNode(scene->GetNodeName(m_selectedEntity) + " (Copy)");
                        // Copy Transform
                        auto srcTransform = scene->GetComponent<Transform>(m_selectedEntity);
                        if (srcTransform) {
                            auto dstTransform = scene->AddComponent<Transform>(newNode);
                            if (dstTransform) {
                                dstTransform->SetPosition(srcTransform->GetPosition());
                                dstTransform->SetRotation(srcTransform->GetRotation());
                                dstTransform->SetScale(srcTransform->GetScale());
                            }
                        }
                        // Copy Camera
                        auto srcCamera = scene->GetComponent<Graphic::Camera>(m_selectedEntity);
                        if (srcCamera) {
                            scene->AddComponent<Graphic::Camera>(newNode);
                        }
                        // Copy RigidBody
                        auto srcRb = scene->GetComponent<RigidBodyComponent>(m_selectedEntity);
                        if (srcRb) {
                            auto dstRb = scene->AddComponent<RigidBodyComponent>(newNode);
                            if (dstRb) {
                                dstRb->SetVelocity(srcRb->GetVelocity());
                            }
                        }
                        // Copy Light
                        auto srcLight = scene->GetComponent<Graphic::LightComponent>(m_selectedEntity);
                        if (srcLight) {
                            auto dstLight = scene->AddComponent<Graphic::LightComponent>(newNode);
                            if (dstLight) {
                                dstLight->SetData(srcLight->GetData());
                            }
                        }
                        // Copy MeshRenderer
                        auto srcMesh = scene->GetComponent<Graphic::MeshRenderer>(m_selectedEntity);
                        if (srcMesh) {
                            auto dstMesh = scene->AddComponent<Graphic::MeshRenderer>(newNode);
                            if (dstMesh) {
                                dstMesh->SetData(srcMesh->GetData());
                            }
                        }
                        m_selectedEntity = newNode;
                        scene->SetDirty(true);
                    }
                }
                if (ImGui::MenuItem("Delete", "Del", false, m_selectedEntity.IsValid())) {
                    if (m_selectedEntity.IsValid()) {
                        scene->RemoveNode(m_selectedEntity);
                        m_selectedEntity = Node{};
                        scene->SetDirty(true);
                    }
                }
                ImGui::EndPopup();
            }
        }
    }
    ImGui::End();

    ImGui::Begin(UI::WINDOW_PROPERTIES);
    auto propsScene = Engine::Get().GetSceneManager()->GetCurrentScene();
    if (m_selectedEntity.IsValid() && propsScene) {
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        std::string nodeName = propsScene->GetNodeName(m_selectedEntity);
        strncpy(buffer, nodeName.c_str(), sizeof(buffer));
        if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
            propsScene->SetNodeName(m_selectedEntity, std::string(buffer));
            propsScene->SetDirty(true);
        }

        ImGui::Separator();

        auto transform = propsScene->GetComponent<Transform>(m_selectedEntity);
        if (transform) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                Vector3 pos = transform->GetPosition();
                if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
                    transform->SetPosition(pos);
                    propsScene->SetDirty(true);
                }

                Vector3 rotation = glm::degrees(glm::eulerAngles(transform->GetRotation()));
                if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f)) {
                    transform->SetRotation(rotation);
                    propsScene->SetDirty(true);
                }

                Vector3 scale = transform->GetScale();
                if (ImGui::DragFloat3("Scale", &scale.x, 0.1f)) {
                    transform->SetScale(scale);
                    propsScene->SetDirty(true);
                }
            }
        }

        // Camera Component display
        auto camera = propsScene->GetComponent<Graphic::Camera>(m_selectedEntity);
        if (camera) {
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
                float fov = glm::degrees(camera->GetFOV());
                if (ImGui::DragFloat("Field of View", &fov, 1.0f, 10.0f, 120.0f)) {
                    camera->SetPerspectiveProjection(
                        glm::radians(fov), camera->GetAspectRatio(), camera->GetNearPlane(), camera->GetFarPlane());
                    propsScene->SetDirty(true);
                }
            }
        }

        // RigidBody Component display
        auto rb = propsScene->GetComponent<RigidBodyComponent>(m_selectedEntity);
        if (rb) {
            if (ImGui::CollapsingHeader("RigidBody", ImGuiTreeNodeFlags_DefaultOpen)) {
                Vector3 vel = rb->GetVelocity();
                if (ImGui::DragFloat3("Velocity", &vel.x, 0.1f)) {
                    rb->SetVelocity(vel);
                    propsScene->SetDirty(true);
                }
            }
        }

        // Light Component display
        auto light = propsScene->GetComponent<Graphic::LightComponent>(m_selectedEntity);
        if (light) {
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto data = light->GetData();
                int typeInt = (int)data.type;
                const char* types[] = {"Directional", "Point", "Spot", "Ambient"};
                if (ImGui::Combo("Type", &typeInt, types, 4)) {
                    data.type = (Graphic::LightComponent::LightType)typeInt;
                    light->SetData(data);
                    propsScene->SetDirty(true);
                }
                if (ImGui::ColorEdit3("Color", data.color.data())) {
                    light->SetData(data);
                    propsScene->SetDirty(true);
                }
                if (ImGui::DragFloat("Intensity", &data.intensity, 0.1f, 0.0f, 100.0f)) {
                    light->SetData(data);
                    propsScene->SetDirty(true);
                }
                if (data.type == Graphic::LightComponent::LightType::Point ||
                    data.type == Graphic::LightComponent::LightType::Spot) {
                    if (ImGui::DragFloat("Range", &data.range, 0.1f, 0.1f, 1000.0f)) {
                        light->SetData(data);
                        propsScene->SetDirty(true);
                    }
                }
                if (data.type == Graphic::LightComponent::LightType::Spot) {
                    if (ImGui::DragFloat("Spot Angle", &data.spotAngle, 1.0f, 1.0f, 179.0f)) {
                        light->SetData(data);
                        propsScene->SetDirty(true);
                    }
                }
            }
        }

        // MeshRenderer Component display
        auto meshRenderer = propsScene->GetComponent<Graphic::MeshRenderer>(m_selectedEntity);
        if (meshRenderer) {
            if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto data = meshRenderer->GetData();
                char meshPath[256];
                strncpy(meshPath, data.meshPath.c_str(), sizeof(meshPath));
                meshPath[sizeof(meshPath) - 1] = '\0';
                if (ImGui::InputText("Mesh", meshPath, sizeof(meshPath))) {
                    data.meshPath = meshPath;
                    meshRenderer->SetData(data);
                    propsScene->SetDirty(true);
                }
                if (ImGui::ColorEdit3("Color", data.color.data())) {
                    meshRenderer->SetData(data);
                    propsScene->SetDirty(true);
                }
                if (ImGui::ColorEdit3("Emissive", data.emissive.data())) {
                    meshRenderer->SetData(data);
                    propsScene->SetDirty(true);
                }
                char matPath[256];
                strncpy(matPath, data.material.c_str(), sizeof(matPath));
                matPath[sizeof(matPath) - 1] = '\0';
                if (ImGui::InputText("Material", matPath, sizeof(matPath))) {
                    data.material = matPath;
                    meshRenderer->SetData(data);
                    propsScene->SetDirty(true);
                }
            }
        }

        // Add Component button
        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("Add Component...", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            ImGui::OpenPopup(UI::POPUP_ADD_COMPONENT);
        }

        if (ImGui::BeginPopup(UI::POPUP_ADD_COMPONENT)) {
            if (ImGui::MenuItem("Camera")) {
                if (!propsScene->GetComponent<Graphic::Camera>(m_selectedEntity)) {
                    propsScene->AddComponent<Graphic::Camera>(m_selectedEntity);
                    propsScene->SetDirty(true);
                }
            }
            if (ImGui::MenuItem("RigidBody")) {
                if (!propsScene->GetComponent<RigidBodyComponent>(m_selectedEntity)) {
                    propsScene->AddComponent<RigidBodyComponent>(m_selectedEntity);
                    propsScene->SetDirty(true);
                }
            }
            if (ImGui::MenuItem("Light")) {
                if (!propsScene->GetComponent<Graphic::LightComponent>(m_selectedEntity)) {
                    propsScene->AddComponent<Graphic::LightComponent>(m_selectedEntity);
                    propsScene->SetDirty(true);
                }
            }
            if (ImGui::MenuItem("Mesh Renderer")) {
                if (!propsScene->GetComponent<Graphic::MeshRenderer>(m_selectedEntity)) {
                    propsScene->AddComponent<Graphic::MeshRenderer>(m_selectedEntity);
                    propsScene->SetDirty(true);
                }
            }
            ImGui::EndPopup();
        }
    } else {
        ImGui::Text("Select an entity to view properties");
    }
    ImGui::End();

    ImGui::Begin(UI::WINDOW_CONTENT_BROWSER);

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

    static float padding       = 16.0f;
    static float thumbnailSize = 128.0f;
    float cellSize             = thumbnailSize + padding;

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount  = (int)(panelWidth / cellSize);
    if (columnCount < 1)
        columnCount = 1;

    ImGui::Columns(columnCount, 0, false);

    for (auto& directoryEntry : std::filesystem::directory_iterator(m_currentAssetDirectory)) {
        const auto& path           = directoryEntry.path();
        std::string filenameString = path.filename().string();

        ImGui::PushID(filenameString.c_str());

        if (directoryEntry.is_directory()) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.7f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        }

        if (ImGui::Button(filenameString.c_str(), {thumbnailSize, thumbnailSize})) {
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

    // Status bar
    {
        ImGuiViewport* viewportBar = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewportBar->Pos.x, viewportBar->Pos.y + viewportBar->Size.y - 25));
        ImGui::SetNextWindowSize(ImVec2(viewportBar->Size.x, 25));
        ImGuiWindowFlags statusFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
        ImGui::Begin("##StatusBar", nullptr, statusFlags);
        auto* sceneManagerStatus = Engine::Get().GetSceneManager();
        size_t entityCount = sceneManagerStatus && sceneManagerStatus->GetCurrentScene()
            ? sceneManagerStatus->GetCurrentScene()->GetNodes().size() : 0;
        ImGui::Text("FPS: %.1f | Frame: %.2f ms | Entities: %zu",
            m_fps, m_frameTime * 1000.0f, entityCount);
        ImGui::SameLine(ImGui::GetWindowWidth() - 150);
        ImGui::Text("PrismaEngine Editor");
        ImGui::End();
    }

    ProfilerPanel::OnImGuiRender();

    // -----------------------------------------------------------------------
    // 文件操作弹窗 (Stubs)
    // -----------------------------------------------------------------------
    auto center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(UI::POPUP_OPEN_PROJECT, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("打开项目 (演示)");
        ImGui::Separator();
        static char projectPath[256] = "assets/projects/Default.prisma";
        ImGui::InputText("路径", projectPath, IM_ARRAYSIZE(projectPath));
        if (ImGui::Button("确定", ImVec2(120, 0))) {
            LOG_INFO("Editor", "正在打开项目: %s", projectPath);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal(UI::POPUP_NEW_PROJECT, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("新建项目");
        ImGui::Separator();
        static char newProjectName[64] = "NewPrismaProject";
        ImGui::InputText("名称", newProjectName, IM_ARRAYSIZE(newProjectName));
        if (ImGui::Button("创建", ImVec2(120, 0))) {
            LOG_INFO("Editor", "正在创建项目: %s", newProjectName);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal(UI::POPUP_OPEN_SCENE, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("打开场景");
        ImGui::Separator();
        static char scenePath[256] = "assets/scenes/Main.scene";
        ImGui::InputText("路径", scenePath, IM_ARRAYSIZE(scenePath));
        if (ImGui::Button("加载", ImVec2(120, 0))) {
            LOG_INFO("Editor", "正在加载场景: %s", scenePath);
            if (auto sceneManager = Engine::Get().GetSceneManager()) {
                sceneManager->LoadFromFile(scenePath);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal(UI::POPUP_SAVE_PROJECT_AS, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("项目另存为");
        ImGui::Separator();
        if (ImGui::Button("保存", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal(UI::POPUP_SAVE_SCENE_AS, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("场景另存为");
        ImGui::Separator();
        static char sceneSavePath[256] = "assets/scenes/Main.scene";
        ImGui::InputText("保存路径", sceneSavePath, IM_ARRAYSIZE(sceneSavePath));
        if (ImGui::Button("保存", ImVec2(120, 0))) {
            LOG_INFO("Editor", "正在保存场景: %s", sceneSavePath);
            if (auto sceneManager = Engine::Get().GetSceneManager()) {
                if (auto* scene = sceneManager->GetCurrentScene()) {
                    scene->Serialize(sceneSavePath);
                }
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void Prisma::EditorLayer::OnEvent(Event& event) {
    if (event.NativeEvent) {
        ImGui_ImplSDL3_ProcessEvent((const SDL_Event*)event.NativeEvent);
    }

    // 检查 ImGui 是否想要捕获此事件。如果是，则标记事件为已处理，防止其传递给下层（如游戏世界）。
    ImGuiIO& io = ImGui::GetIO();
    event.Handled |= event.IsInCategory(EventCategoryMouse) && io.WantCaptureMouse;
    event.Handled |= event.IsInCategory(EventCategoryKeyboard) && io.WantCaptureKeyboard;
}
