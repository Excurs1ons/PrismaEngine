#include "MetroidvaniaApp.h"
#include "graphic/Renderer2D.h"
#include "graphic/Renderer.h"
#include "graphic/OrthographicCamera.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "core/EntityManager.h"
#include "core/Event.h"
#include "input/InputManager.h"
#include "Logger.h"
#include <vector>
#include <SDL3/SDL_scancode.h>

namespace Prisma {

using Prisma::Input::KeyCode;

// ============================================================
// 模拟输入脚本：每个 {frame, key, press} 三元组
// ============================================================
struct SimStep { int frame; KeyCode key; bool press; };

static const SimStep kTestScript[] = {
    // 初始化等待（让 C# 脚本 Bootstrap 完成，物理稳定）
    { 60,  KeyCode::D,          true  },  // 0. 开始向右移动
    { 200, KeyCode::Space,      true  },  // 1. 跳跃上第一个平台
    { 205, KeyCode::Space,      false },
    { 350, KeyCode::Space,      true  },  // 2. 跳跃
    { 355, KeyCode::Space,      false },
    { 450, KeyCode::LShift,     true  },  // 3. 冲刺越过缝隙 (Room 2)
    { 455, KeyCode::LShift,     false },
    { 550, KeyCode::Space,      true  },  // 4. 跳跃
    { 555, KeyCode::Space,      false },
    { 600, KeyCode::LShift,     true  },  // 5. 再冲刺
    { 605, KeyCode::LShift,     false },
    { 700, KeyCode::D,          false },  // 6. 停止移动，等自动退出
};

MetroidvaniaApp::MetroidvaniaApp()
    : Application({"MetroidvaniaDemo", "", 1024, 896, true, true, Graphic::PresentMode::Mailbox, 0})
{
}

int MetroidvaniaApp::OnInitialize() {
    LOG_INFO("Metroidvania", "Initializing C# scripted demo, resolution={0}x{1} simInput={2} autoExit={3}s",
             m_Spec.Width, m_Spec.Height, m_simInput, m_autoExitTimeout);
    return 0;
}

void MetroidvaniaApp::OnUpdate(Timestep ts) {
    m_elapsedTime += ts;
    m_simFrame++;

    // === 模拟输入：按脚本注入按键 ===
    if (m_simInput) {
        int totalSteps = sizeof(kTestScript) / sizeof(kTestScript[0]);
        for (int i = 0; i < totalSteps; i++) {
            if (kTestScript[i].frame == m_simFrame) {
                auto* input = Engine::Get().GetInputManager();
                if (input) {
                    LOG_INFO("Metroidvania", "[SIM] frame={0} key={1} press={2}", 
                             m_simFrame, static_cast<uint32_t>(kTestScript[i].key),
                             kTestScript[i].press);
                    input->SetKeyState(kTestScript[i].key, kTestScript[i].press);
                }
            }
        }
    }

    // === 自动退出 ===
    if (m_elapsedTime >= m_autoExitTimeout) {
        LOG_INFO("Metroidvania", "Auto-exit after {0:.1f}s (sim={1})", m_elapsedTime, m_simInput);
        Close();
        return;
    }
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

        // 模拟输入状态提示
        if (m_simInput) {
            Graphic::Renderer2D::DrawString("[SIM] auto-test mode (FPS: " +
                                            std::to_string((int)Engine::Get().GetFPS()) + ")",
                                            hud(30.0f, 100.0f), 2.0f, {1.0f, 0.8f, 0.2f, 1.0f});
        }
    }
}

void MetroidvaniaApp::OnEvent(Event& e) {
    Application::OnEvent(e);

    EventDispatcher d(e);
    d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        if (ev.GetKeyCode() == SDL_SCANCODE_ESCAPE) { 
            LOG_INFO("Metroidvania", "ESC pressed — closing");
            Close(); return true; 
        }
        return false;
    });
}

} // namespace Prisma
