#include "EditorLayer.h"
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

    // 检查是否有 ViewportRenderPass
    if (!m_viewportRenderPass || !m_viewportRenderPass->IsInitialized() ||
        !m_viewportTexture || !m_viewportDepthTexture) {
        // 如果 ViewportRenderPass 未初始化，直接渲染到 SwapChain（后备方案）
        renderSystem->RenderScene(scene, camera);
        return;
    }

    // 获取 Vulkan 设备和命令缓冲区
    auto vkDevice = static_cast<Graphic::Vulkan::RenderDeviceVulkan*>(renderSystem->GetDevice());
    if (!vkDevice) {
        return;
    }

    VkCommandBuffer cmd = vkDevice->GetCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) {
        return;
    }

    // 开始 Viewport RenderPass（渲染到离屏纹理）
    m_viewportRenderPass->Begin(cmd);

    // 渲染场景
    // [改动] 传递 m_viewportTexture 以便管线知道输出目标
    renderSystem->RenderScene(scene, camera, m_viewportTexture.get());

    // 结束 Viewport RenderPass
    m_viewportRenderPass->End(cmd);
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
            if (ImGui::MenuItem("ImGui Demo Window", nullptr, &m_showDemoWindow)) {
            }
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

    ImGui::Begin("Scene Hierarchy");
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
                    camera->SetPerspectiveProjection(
                        glm::radians(fov), camera->GetAspectRatio(), camera->GetNearPlane(), camera->GetFarPlane());
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
