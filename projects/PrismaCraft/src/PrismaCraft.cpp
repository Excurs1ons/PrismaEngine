#include "../engine/Application.h"
#include "../engine/Engine.h"
#include "../engine/core/Layer.h"
#include "../engine/Logger.h"
#include "../engine/core/Timestep.h"
#include "../engine/graphic/Renderer.h" // 包含现代渲染器
#include "../engine/graphic/Mesh.h"
#include "../engine/graphic/Material.h"
#include "../engine/SceneManager.h"
#include "../engine/Scene.h"
#include "../engine/Camera.h"

namespace Prisma {

/**
 * @brief 现代风格的游戏逻辑层
 */
class GameplayLayer : public Layer {
public:
    GameplayLayer() : Layer("GameplayLayer") {}

    void OnAttach() override {
        LOG_INFO("Gameplay", "GameplayLayer Attached!");
        
        // 模拟加载资源 (以后会走 AssetManager)
        // m_CubeMesh = AssetManager::Get().Load<Mesh>("cube.fbx");
        // m_WoodMaterial = AssetManager::Get().Load<Material>("wood.mat");
    }

    void OnUpdate(Timestep ts) override {
        // 更新物体变换
        m_CubeRotation += ts.GetSeconds() * 0.5f;
    }

    /**
     * @brief 准备渲染数据并提交 (OnRender 替代 PrepareRenderData)
     */
    void OnRender() override {
        // 1. 获取相机信息并填入上下文
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

        // 2. 开启 Renderer 的场景收集
        Graphic::Renderer::BeginScene(cameraData);

        // 3. 提交渲染指令 (模拟提交一个旋转的方块)
        // 在真实项目中，这里会遍历 Scene 的 Renderables
        // PrismaMath::mat4 transform = PrismaMath::translate(PrismaMath::mat4(1.0f), {0, 0, -5});
        // transform = PrismaMath::rotate(transform, m_CubeRotation, {0, 1, 0});
        
        // Renderer::Submit(m_CubeMesh.get(), m_WoodMaterial.get(), transform);

        // 4. 结束收集，准备执行
        Graphic::Renderer::EndScene();
    }

private:
    float m_CubeRotation = 0.0f;
    // std::shared_ptr<Graphic::Mesh> m_CubeMesh;
    // std::shared_ptr<Graphic::Material> m_WoodMaterial;
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
