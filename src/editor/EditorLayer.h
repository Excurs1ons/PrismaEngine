#pragma once

#include "../engine/core/Layer.h"
#include "../engine/Application.h"
#include "../engine/graphic/RenderSystem.h"
#include "../engine/SceneManager.h"
#include "../engine/Scene.h"
#include "../engine/Camera.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <SDL3/SDL.h>

namespace Prisma {

class EditorLayer : public Layer {
public:
    EditorLayer() : Layer("EditorLayer") {}

    void OnUpdate(Timestep ts) override {
        // 更新逻辑
    }

    void OnRender() override {
        auto renderSystem = Graphic::RenderSystem::Get();
        
        // 渲染场景 (推送模式)
        auto sceneManager = SceneManager::Get();
        if (sceneManager) {
            auto* scene = sceneManager->GetCurrentScene();
            if (scene) {
                auto camera = scene->GetMainCamera();
                if (camera) {
                    renderSystem->RenderScene(scene, camera.get());
                }
            }
        }
    }

    void OnImGuiRender() override {
        // 这里可以放主菜单等 UI
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("文件")) {
                if (ImGui::MenuItem("退出", "Alt+F4")) {
                    Application::Get().Close();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        ImGui::ShowDemoWindow();
    }

    void OnEvent(Event& event) override {
        if (event.NativeEvent) {
            ImGui_ImplSDL3_ProcessEvent((const SDL_Event*)event.NativeEvent);
        }
    }
};

} // namespace Prisma
