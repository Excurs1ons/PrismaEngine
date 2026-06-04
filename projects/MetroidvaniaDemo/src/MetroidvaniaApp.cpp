#include "MetroidvaniaApp.h"
#include "graphic/Renderer2D.h"
#include "graphic/Renderer.h"
#include "graphic/OrthographicCamera.h"
#include "graphic/interfaces/IResourceManager.h"
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

    // 创建程序化精灵纹理（1x1 纯色纹理）
    auto* resMgr = Engine::Get().GetRenderResourceManager();
    if (resMgr) {
        auto makeTexture = [resMgr](uint32_t rgba) -> std::shared_ptr<Graphic::ITexture> {
            Graphic::TextureDesc desc;
            desc.width = 1;
            desc.height = 1;
            desc.format = Graphic::TextureFormat::RGBA8_UNorm;
            return resMgr->CreateTextureFromMemory(&rgba, sizeof(rgba), desc);
        };
        // Player: blue rgba(0,100,255,255)
        m_playerTexture = makeTexture(0xFFFF6400);
        // Enemy: red rgba(255,50,50,255)
        m_enemyTexture = makeTexture(0xFF3232FF);
        // DashPickup: yellow-green rgba(150,255,50,255)
        m_dashPickupTexture = makeTexture(0xFF32FF96);
        // AbilityGate: grey rgba(128,128,128,255)
        m_abilityGateTexture = makeTexture(0xFF808080);
        // White texture for fallback
        m_whiteTexture = makeTexture(0xFFFFFFFF);
        LOG_INFO("Metroidvania", "Created {} sprite textures", 5);
    } else {
        LOG_WARNING("Metroidvania", "No render resource manager - sprite textures disabled");
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

    // 从 SoA 数据池读取实体位置，用精灵纹理绘制
    auto& em = EntityManager::Get();
    auto* rb = em.GetRenderData();
    auto* tb = em.GetTransformRead();
    uint32_t count = em.GetAliveCount();

    for (uint32_t i = 0; i < count; ++i) {
        if (!rb->active[i]) continue;

        // 根据实体尺寸匹配对应纹理
        std::shared_ptr<Graphic::ITexture> tex;
        float w = rb->sizeW[i];
        float h = rb->sizeH[i];
        if (w >= 12.0f && w <= 13.0f && h >= 16.0f && h <= 17.0f)      tex = m_playerTexture;
        else if (w >= 14.0f && w <= 15.0f && h >= 14.0f && h <= 15.0f) tex = m_enemyTexture;
        else if (w >= 12.0f && w <= 13.0f && h >= 12.0f && h <= 13.0f) tex = m_dashPickupTexture;
        else if (w >= 32.0f && w <= 33.0f && h >= 32.0f && h <= 33.0f) tex = m_abilityGateTexture;
        else                                                              tex = m_whiteTexture;

        Graphic::Renderer2D::DrawQuad(
            Vector2{tb->posX[i], tb->posY[i]},
            Vector2{rb->sizeW[i], rb->sizeH[i]},
            tex
        );
    }

    // 显式红色方块测试（验证管线shader是否工作）
    Graphic::Renderer2D::DrawQuad(Vector2{128, 112}, Vector2{32, 32}, {1.0f, 0.0f, 0.0f, 1.0f});
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
        // Forward real keyboard presses to InputManager for C# scripts
        auto* input = Engine::Get().GetInputManager();
        if (input) {
            input->SetKeyState(static_cast<KeyCode>(ev.GetKeyCode()), true);
        }
        return false;
    });

    d.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent& ev) {
        auto* input = Engine::Get().GetInputManager();
        if (input) {
            input->SetKeyState(static_cast<KeyCode>(ev.GetKeyCode()), false);
        }
        return false;
    });
}

} // namespace Prisma
