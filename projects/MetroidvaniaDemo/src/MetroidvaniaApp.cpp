#include "MetroidvaniaApp.h"
#include "graphic/Renderer2D.h"
#include "graphic/Renderer.h"
#include "graphic/OrthographicCamera.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "core/EntityManager.h"
#include "core/Event.h"
#include "Logger.h"
#include <SDL3/SDL_scancode.h>

namespace Prisma {

MetroidvaniaApp::MetroidvaniaApp()
    : Application({"MetroidvaniaDemo", "", 1024, 896, true, true, Graphic::PresentMode::Mailbox, 0})
{
}

int MetroidvaniaApp::OnInitialize() {
    LOG_INFO("Metroidvania", "Initializing C# scripted demo, resolution={0}x{1}",
             m_Spec.Width, m_Spec.Height);
    return 0;
}

void MetroidvaniaApp::OnUpdate(Timestep ts) {
    (void)ts;
    // 所有游戏逻辑由 C# 脚本处理
}

void MetroidvaniaApp::OnRender() {
    auto* scene = Engine::Get().GetSceneManager()->GetCurrentScene();
    auto camera = scene ? scene->GetMainCamera() : nullptr;
    auto ortho = std::dynamic_pointer_cast<Graphic::OrthographicCamera>(camera);
    if (!ortho) return;

    // 从 ScriptEngine 同步相机位置
    float camX = 0, camY = 0;
#if PRISMA_ENABLE_SCRIPTING
    Engine::Get().GetScriptEngine().GetCameraPos(&camX, &camY);
#endif
    ortho->SetPosition({camX, camY});

    Graphic::Renderer2D::BeginScene(*ortho);
    Graphic::Renderer2D::DrawNodesSoA();
    Graphic::Renderer2D::EndScene();

    // HUD 覆盖层
    {
        Vector3 camPos = ortho->GetPosition();
        auto hud = [&](float sx, float sy) { return Vector2{sx + camPos.x, sy + camPos.y}; };

        uint32_t totalNodes = Engine::Get().GetEntityManager().GetAliveCount();
        std::string fpsInfo = std::to_string((int)Engine::Get().GetFPS()) + " FPS | Nodes: " +
                              std::to_string(totalNodes);
        Graphic::Renderer2D::DrawString(fpsInfo, hud(30.0f, 30.0f), 2.0f,
                                        {0.6f, 0.6f, 0.6f, 1.0f});
        Graphic::Renderer2D::DrawString("Cam: (" + std::to_string((int)camX) + "," +
                                        std::to_string((int)camY) + ")",
                                        hud(30.0f, 65.0f), 1.5f, {0.6f, 0.6f, 0.9f, 1.0f});
    }
}

void MetroidvaniaApp::OnEvent(Event& e) {
    Application::OnEvent(e);

    EventDispatcher d(e);
    d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        if (ev.GetKeyCode() == SDL_SCANCODE_ESCAPE) { Close(); return true; }
        return false;
    });
}

} // namespace Prisma
