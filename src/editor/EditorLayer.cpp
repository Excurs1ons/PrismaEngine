#include "EditorLayer.h"
#include "UIStrings.h"
#include "graphic/ImGuiVulkanResourceManager.h"
#include "graphic/ViewportRenderPass.h"

Prisma::EditorLayer::EditorLayer() : Layer("EditorLayer") {
    m_editorCameraObject = std::make_shared<GameObject>("Editor Camera");
    m_editorCamera       = m_editorCameraObject->AddComponent<Graphic::Camera>();
    m_editorCamera->SetPerspectiveProjection(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
    m_editorCameraObject->GetTransform()->SetPosition({0, 2, 5});
}

void Prisma::EditorLayer::OnUpdate(Timestep ts) {
    if (!m_viewportHovered || !ImGui::IsMouseDown(ImGuiMouseButton_Right))
        return;

    float dt       = ts.GetSeconds();
    auto transform = m_editorCameraObject->GetTransform();
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

    // [修复] 移除所有手动的 ViewportRenderPass->Begin/End 调用。
    // 原因：这些操作会开启一个与引擎默认或管线内部冲突的 RenderPass，导致驱动崩溃。
    // 我们只需将 m_viewportTexture 传给渲染系统，让管线内部负责输出目标的重定向。
    renderSystem->RenderScene(scene, camera, m_viewportTexture.get());
}

void Prisma::EditorLayer::OnImGuiRender() {
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
                        m_selectedEntity = nullptr;
                        LOG_INFO("Editor", "新场景已创建");
                    }
                }
                if (ImGui::MenuItem(UI::ITEM_OPEN_SCENE, "Ctrl+Shift+O")) {
                    ImGui::OpenPopup(UI::POPUP_OPEN_SCENE);
                }
                ImGui::Separator();
                if (ImGui::MenuItem(UI::ITEM_SAVE_SCENE, "Ctrl+Alt+S")) {
                    LOG_INFO("Editor", "场景已保存 (暂存实现)");
                    if (auto sceneManager = Engine::Get().GetSceneManager()) {
                        if (auto scene = sceneManager->GetCurrentScene()) {
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
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();

    // Viewport Panel
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin(UI::WINDOW_VIEWPORT);

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
                    auto vkTexture = dynamic_cast<Graphic::Vulkan::VulkanTexture*>(m_viewportTexture.get());
                    if (vkTexture) {
                        vkTexture->SetDebugName("Viewport Color Texture");
                    }

                    // 创建深度纹理
                    Graphic::TextureDesc depthDesc;
                    depthDesc.width              = (uint32_t)m_viewportSize.x;
                    depthDesc.height             = (uint32_t)m_viewportSize.y;
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
                            m_viewportRenderPass->Initialize(
                                vkDevice->GetVkDevice(),
                                vkTexture->GetVkImageView(),
                                vkDepthTexture->GetVkImageView(),
                                (uint32_t)m_viewportSize.x,
                                (uint32_t)m_viewportSize.y
                            );
                        }
                    }

                    // 创建 ImGui descriptor set
                    m_viewportDescriptorSet = VK_NULL_HANDLE;
                    auto& editor = Editor::Get();
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
            if (scene->IsDirty()) hierarchyTitle += "*";
        }
    }
    ImGui::Begin(hierarchyTitle.c_str());

    if (auto sceneManager = Engine::Get().GetSceneManager()) {
        if (auto scene = sceneManager->GetCurrentScene()) {
            auto& objects = scene->GetGameObjects();
            for (auto& obj : objects) {
                ImGuiTreeNodeFlags flags =
                    ((m_selectedEntity == obj) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
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

    ImGui::Begin(UI::WINDOW_PROPERTIES);
    if (m_selectedEntity) {
        auto scene = Engine::Get().GetSceneManager()->GetCurrentScene();
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, m_selectedEntity->name.c_str(), sizeof(buffer));
        if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
            m_selectedEntity->name = std::string(buffer);
            if (scene) scene->SetDirty(true);
        }

        ImGui::Separator();

        auto transform = m_selectedEntity->GetTransform();
        if (transform) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                Vector3 pos = transform->GetPosition();
                if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
                    transform->SetPosition(pos);
                    if (scene) scene->SetDirty(true);
                }

                Vector3 rotation = glm::degrees(glm::eulerAngles(transform->GetRotation()));
                if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f)) {
                    transform->SetRotation(rotation);
                    if (scene) scene->SetDirty(true);
                }

                Vector3 scale = transform->GetScale();
                if (ImGui::DragFloat3("Scale", &scale.x, 0.1f)) {
                    transform->SetScale(scale);
                    if (scene) scene->SetDirty(true);
                }
            }
        }

        // Camera Component display
        auto camera = m_selectedEntity->GetComponent<Graphic::Camera>();
        if (camera) {
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
                float fov = glm::degrees(camera->GetFOV());
                if (ImGui::DragFloat("Field of View", &fov, 1.0f, 10.0f, 120.0f)) {
                    camera->SetPerspectiveProjection(
                        glm::radians(fov), camera->GetAspectRatio(), camera->GetNearPlane(), camera->GetFarPlane());
                    if (scene) scene->SetDirty(true);
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
                    if (scene) scene->SetDirty(true);
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
                if (!m_selectedEntity->GetComponent<Graphic::Camera>())
                    m_selectedEntity->AddComponent<Graphic::Camera>();
            }
            if (ImGui::MenuItem("RigidBody")) {
                if (!m_selectedEntity->GetComponent<RigidBodyComponent>())
                    m_selectedEntity->AddComponent<Graphic::Camera>();
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
        if (ImGui::Button("取消", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
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
        if (ImGui::Button("取消", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal(UI::POPUP_OPEN_SCENE, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("打开场景");
        ImGui::Separator();
        static char scenePath[256] = "assets/scenes/Main.scene";
        ImGui::InputText("路径", scenePath, IM_ARRAYSIZE(scenePath));
        if (ImGui::Button("加载", ImVec2(120, 0))) {
            LOG_INFO("Editor", "正在加载场景: %s", scenePath);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal(UI::POPUP_SAVE_PROJECT_AS, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("项目另存为");
        ImGui::Separator();
        if (ImGui::Button("保存", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal(UI::POPUP_SAVE_SCENE_AS, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("场景另存为");
        ImGui::Separator();
        if (ImGui::Button("保存", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
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
