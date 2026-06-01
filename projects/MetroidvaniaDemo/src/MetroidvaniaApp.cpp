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

    auto* sceneMgr = Engine::Get().GetSceneManager();
    if (!sceneMgr->GetCurrentScene()) {
        sceneMgr->CreateNewScene();
    }

    // 加载地砖地图，使用引擎内置 1x1 白色纹理作为视觉 fallback
    m_tilemap = std::make_shared<Tilemap::Tilemap>();
    if (m_tilemap->LoadFromJSON("assets/maps/test_dungeon.json")) {
        m_tilemapRenderer.SetTilemap(m_tilemap);
        m_tilemapRenderer.SetTexture(Graphic::Renderer2D::GetWhiteTexture());
        LOG_INFO("Metroidvania", "Tilemap loaded: {}x{} tiles, {} layers",
                 m_tilemap->GetWidth(), m_tilemap->GetHeight(),
                 m_tilemap->GetLayerCount());
    } else {
        LOG_WARNING("Metroidvania", "Failed to load tilemap, tile rendering disabled");
        m_tilemap.reset();
    }

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
    float camX = 0, camY = 0;
#if PRISMA_ENABLE_SCRIPTING
    Engine::Get().GetScriptEngine().GetCameraPos(&camX, &camY);
#endif
    Graphic::OrthographicCamera ortho(
        camX - 128, camX + 128,
        camY + 112, camY - 112,
        -1000.0f, 1000.0f
    );

    Graphic::Renderer2D::BeginScene(ortho);
    // 显式红色方块测试（验证管线shader是否工作）
    Graphic::Renderer2D::DrawQuad(Vector2{128, 112}, Vector2{32, 32}, {1.0f, 0.0f, 0.0f, 1.0f});
    Graphic::Renderer2D::DrawNodesSoA();
    if (m_tilemap) m_tilemapRenderer.Render(ortho);
    Graphic::Renderer2D::EndScene();
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
