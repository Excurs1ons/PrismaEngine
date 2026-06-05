#include "MetroidvaniaApp.h"
#include "graphic/Renderer2D.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "transform/Camera.h"
#include "core/Event.h"
#include "core/Node.h"
#include "core/EntityManager.h"
#include "core/SpriteRendererComponent.h"
#include "physics/PhysicsSystem.h"
#include "physics/RigidBody.h"
#include "Logger.h"
#include <SDL3/SDL_scancode.h>

namespace Prisma {

MetroidvaniaApp::MetroidvaniaApp()
    : Application({"MetroidvaniaDemo", "", 1024, 896, true, true, Graphic::PresentMode::Mailbox, 0})
{
}

int MetroidvaniaApp::OnInitialize() {
    LOG_INFO("Metroidvania", "Scene-driven rendering initialized");

    // 场景由引擎从 entryScene 配置自动加载
    auto* sceneMgr = Engine::Get().GetSceneManager();
    auto* scene = sceneMgr ? sceneMgr->GetCurrentScene() : nullptr;

    // 如果 entryScene 加载失败，回退创建空场景
    if (!scene) {
        sceneMgr->CreateNewScene();
        scene = sceneMgr->GetCurrentScene();
    }

    // 确保相机视口与窗口同步
    if (scene) {
        auto camera = scene->GetMainCamera();
        if (camera) {
            camera->SetViewport(m_Spec.Width, m_Spec.Height);
        }
    }

    // 创建测试场景物体
    {
        // 红色方块 - 动态（受重力下落）
        m_redNode = scene->CreateNode("Block_Red");
        m_redNode.SetPosition({-200.0f, 150.0f});
        Core::SpriteRendererComponent sprite1;
        sprite1.color = {1.0f, 0.2f, 0.2f, 1.0f};
        sprite1.size = {64.0f, 64.0f};
        sprite1.WriteToSoA(m_redNode.GetIndex());

        // 绿色方块 - 中间（静态参考）
        auto node2 = scene->CreateNode("Block_Green");
        node2.SetPosition({0.0f, 0.0f});
        Core::SpriteRendererComponent sprite2;
        sprite2.color = {0.2f, 1.0f, 0.2f, 1.0f};
        sprite2.size = {64.0f, 64.0f};
        sprite2.WriteToSoA(node2.GetIndex());

        // 蓝色方块 - 右下（静态参考）
        auto node3 = scene->CreateNode("Block_Blue");
        node3.SetPosition({200.0f, -150.0f});
        Core::SpriteRendererComponent sprite3;
        sprite3.color = {0.2f, 0.4f, 1.0f, 1.0f};
        sprite3.size = {64.0f, 64.0f};
        sprite3.WriteToSoA(node3.GetIndex());

        // 双缓冲同步
        EntityManager::Get().SwapBuffers();
        m_redNode.SetPosition({-200.0f, 150.0f});
        node2.SetPosition({0.0f, 0.0f});
        node3.SetPosition({200.0f, -150.0f});
    }

    // 初始化物理：红色方块动态下落 + 地面
    {
        auto* physics = Engine::Get().GetPhysicsSystem();
        if (physics) {
            physics->setGravity({0.0, -300.0, 0.0});  // 2D 像素空间重力

            // 红色方块 - 动态刚体
            m_redBody = physics->createRigidBody(Physics::RigidBodyType::Dynamic);
            m_redBody->setPosition({-200.0, 150.0, 0.0});
            m_redBody->setShapeType(Physics::CollisionShapeType::Box);
            m_redBody->setCollisionHalfSize({32.0, 32.0, 1.0});  // 64x64 像素方块
            m_redBody->setMass(1.0);
            m_redBody->setUserData(static_cast<void*>(&m_redNode));

            // 地面 - 静态刚体（屏幕底部）
            auto* ground = physics->createRigidBody(Physics::RigidBodyType::Static);
            ground->setPosition({0.0, -400.0, 0.0});
            ground->setShapeType(Physics::CollisionShapeType::Box);
            ground->setCollisionHalfSize({600.0, 16.0, 1.0});  // 宽地面
        }
    }

    return 0;
}

void MetroidvaniaApp::OnUpdate(Timestep ts) {
    m_elapsedTime += ts;

    // 每帧同步物理位置到渲染节点
    SyncPhysicsToNodes();

    if (m_elapsedTime >= m_autoExitTimeout) {
        LOG_INFO("Metroidvania", "Auto-exit after {0:.1f}s", m_elapsedTime);
        Close();
    }
}

void MetroidvaniaApp::OnRender() {
    // 场景相机驱动渲染（正交模式 + 暗蓝清屏色由场景文件配置）
    auto* scene = Engine::Get().GetSceneManager()->GetCurrentScene();
    auto camera = scene ? scene->GetMainCamera() : nullptr;
    if (camera) {
        auto cam = std::dynamic_pointer_cast<Graphic::Camera>(camera);
        if (cam) {
            cam->SetViewport(m_Spec.Width, m_Spec.Height);
        }
        Graphic::Renderer2D::BeginScene(*camera);

        // 渲染场景中所有活跃实体
        Graphic::Renderer2D::DrawNodesSoA();
    }
    Graphic::Renderer2D::EndScene();
}

void MetroidvaniaApp::OnEvent(Event& e) {
    Application::OnEvent(e);

    EventDispatcher d(e);
    d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        if (ev.GetKeyCode() == SDL_SCANCODE_ESCAPE) {
            LOG_INFO("Metroidvania", "ESC pressed — closing");
            Close();
            return true;
        }
        return false;
    });
}

void MetroidvaniaApp::SyncPhysicsToNodes() {
    if (!m_redBody) return;

    // 同步红色方块物理位置到渲染节点
    auto pos = m_redBody->getPosition();
    m_redNode.SetPosition({static_cast<float>(pos.x), static_cast<float>(pos.y)});
}

} // namespace Prisma