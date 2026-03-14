#include "../engine/Application.h"
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
     * @brief 准备渲染数据 (这里是重点！)
     */
    void PrepareRenderData(Graphic::RenderContext& ctx) {
        // 1. 获取相机信息并填入上下文
        auto* scene = SceneManager::Get()->GetCurrentScene();
        if (scene) {
            auto camera = scene->GetMainCamera();
            if (camera) {
                ctx.camera.viewMatrix = camera->GetViewMatrix();
                ctx.camera.projectionMatrix = camera->GetProjectionMatrix();
                ctx.camera.position = camera->GetPosition();
            }
        }

        // 2. 开启 Renderer 的场景收集
        Graphic::Renderer::BeginScene(ctx.camera);

        // 3. 提交渲染指令 (模拟提交一个旋转的方块)
        // 在真实项目中，这里会遍历 Scene 的 Renderables
        PrismaMath::mat4 transform = PrismaMath::translate(PrismaMath::mat4(1.0f), {0, 0, -5});
        transform = PrismaMath::rotate(transform, m_CubeRotation, {0, 1, 0});
        
        // Renderer::Submit(m_CubeMesh.get(), m_WoodMaterial.get(), transform);

        // 4. 结束收集，准备执行
        Graphic::Renderer::EndScene();
    }

    void OnRender() override {
        // 以前这里是手动调渲染，现在这里什么都不用做！
        // 因为渲染流程已经被 Engine 和 Pipeline 接管了。
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

    /**
     * @brief 转发渲染准备请求到逻辑层
     */
    void PrepareRenderData(Graphic::RenderContext& ctx) override {
        if (m_GameplayLayer) {
            m_GameplayLayer->PrepareRenderData(ctx);
        }
    }

    void OnShutdown() override {
        LOG_INFO("Game", "Shutting down PrismaCraft...");
    }

private:
    GameplayLayer* m_GameplayLayer = nullptr;
};

} // namespace Prisma

extern "C" {
    ENGINE_API Prisma::Application* CreateApplication() {
        return new Prisma::PrismaCraft();
    }
}
