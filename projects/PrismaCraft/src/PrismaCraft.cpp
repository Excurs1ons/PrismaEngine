#include "app/Application.h"
#include "app/Engine.h"
#include "core/Layer.h"
#include "logger/Logger.h"
#include "core/Timestep.h"
#include "graphic/Renderer.h"
#include "graphic/Mesh.h"
#include "graphic/Material.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "transform/Camera.h"

namespace Prisma {

/**
 * @brief 现代风格的游戏逻辑层
 */
class GameplayLayer : public Layer {
public:
    GameplayLayer() : Layer("GameplayLayer") {}

    void OnAttach() override {
        LOG_INFO("Gameplay", "GameplayLayer Attached!");
    }

    void OnUpdate(Timestep ts) override {
        m_CubeRotation += ts.GetSeconds() * 0.5f;
    }

    /**
     * @brief 准备渲染数据并提交
     */
    void OnRender() override {
        Graphic::CameraData cameraData;
        auto* scene = Engine::Get().GetSceneManager()->GetCurrentScene();
        if (scene) {
            auto camera = scene->GetMainCamera();
            if (camera) {
                cameraData.viewMatrix = camera->GetViewMatrix();
                cameraData.projectionMatrix = camera->GetProjectionMatrix();
                cameraData.position = camera->GetPosition();
            }
        }

        Graphic::Renderer::BeginScene(cameraData);
        Graphic::Renderer::EndScene();
    }

private:
    float m_CubeRotation = 0.0f;
};

/**
 * @brief PrismaCraft 应用程序实现
 */
class PrismaCraft : public Application {
public:
    PrismaCraft() {
        LOG_INFO("Game", "PrismaCraft Created");
    }

    ~PrismaCraft() override = default;

    int OnInitialize() override {
        m_GameplayLayer = new GameplayLayer();
        PushLayer(m_GameplayLayer);
        return 0;
    }

    void OnShutdown() override {
        LOG_INFO("Game", "Shutting down PrismaCraft...");
    }

private:
    GameplayLayer* m_GameplayLayer = nullptr;
};

} // namespace Prisma

#ifdef _WIN32
#define PRISMACRAFT_API __declspec(dllexport)
#else
#define PRISMACRAFT_API __attribute__((visibility("default")))
#endif

extern "C" PRISMACRAFT_API Prisma::Application* CreateApplication() {
    return new Prisma::PrismaCraft();
}
