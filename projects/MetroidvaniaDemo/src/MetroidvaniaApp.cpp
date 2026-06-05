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
#include "Logger.h"
#include <SDL3/SDL_scancode.h>

namespace Prisma {

MetroidvaniaApp::MetroidvaniaApp()
    : Application({"MetroidvaniaDemo", "", 1024, 896, true, true, Graphic::PresentMode::Mailbox, 0})
{
}

int MetroidvaniaApp::OnInitialize() {
    LOG_INFO("Metroidvania", "Scene-driven demo initialized");

    auto* sceneMgr = Engine::Get().GetSceneManager();
    auto* scene = sceneMgr ? sceneMgr->GetCurrentScene() : nullptr;

    if (!scene) {
        sceneMgr->CreateNewScene();
        scene = sceneMgr->GetCurrentScene();
    }

    if (scene) {
        auto camera = scene->GetMainCamera();
        if (camera) {
            camera->SetViewport(m_Spec.Width, m_Spec.Height);
        }
    }

    // 设置物理系统配置
    auto* physics = Engine::Get().GetPhysicsSystem();
    if (physics) {
        physics->setGravity({0.0, -900.0, 0.0}); // 较强的重力以获得更好的手感
    }

    return 0;
}

void MetroidvaniaApp::OnUpdate(Timestep ts) {
    if (m_autoQuit) {
        m_elapsedTime += ts;
        if (m_elapsedTime >= m_autoExitTimeout) {
            LOG_INFO("Metroidvania", "Auto-exit after {0:.1f}s", m_elapsedTime);
            Close();
        }
    }
}

void MetroidvaniaApp::OnRender() {
    auto* scene = Engine::Get().GetSceneManager()->GetCurrentScene();
    auto camera = scene ? scene->GetMainCamera() : nullptr;
    if (camera) {
        auto cam = std::dynamic_pointer_cast<Graphic::Camera>(camera);
        if (cam) {
            cam->SetViewport(m_Spec.Width, m_Spec.Height);
        }
        Graphic::Renderer2D::BeginScene(*camera);
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

} // namespace Prisma
