#include "Template2DApp.h"
#include "graphic/Renderer2D.h"
#include "graphic/Renderer.h"
#include "graphic/OrthographicCamera.h"
#include "graphic/SpriteRenderer.h"
#include "app/Engine.h"
#include "SceneManager.h"
#include "scene/Scene.h"
#include "core/Event.h"
#include "Logger.h"
#include <SDL3/SDL_scancode.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

namespace Prisma {

// ============================================================================
// Template2DApp 实现
// ============================================================================

Template2DApp::Template2DApp()
    : Application({"Template2D", "", 1920, 1080, true, true, Graphic::PresentMode::Mailbox, 0})
{
}

int Template2DApp::OnInitialize() {
    LOG_INFO("Template2D", "C# 脚本化启动, 分辨率={0}x{1}", m_Spec.Width, m_Spec.Height);
    return 0;
}

void Template2DApp::OnUpdate(Timestep ts) {
    // 所有游戏逻辑由 C# 脚本处理
    (void)ts;
}

void Template2DApp::OnRender() {
    auto* scene = Engine::Get().GetSceneManager()->GetCurrentScene();
    auto camera = scene ? scene->GetMainCamera() : nullptr;
    auto ortho = std::dynamic_pointer_cast<Graphic::OrthographicCamera>(camera);
    if (!ortho) return;

    // 从 ScriptEngine 同步相机位置到渲染相机
    float camX = 0, camY = 0;
    Engine::Get().GetScriptEngine().GetCameraPos(&camX, &camY);
    ortho->SetPosition({camX, camY});

    Graphic::Renderer2D::BeginScene(*ortho);

    float winW = (float)m_Spec.Width, winH = (float)m_Spec.Height;
    const float step = 50.0f;

    // 网格
    for (float x = 0.0f; x <= winW; x += step) {
        float a = ((int)x % 100 == 0) ? 0.15f : 0.08f;
        Graphic::Renderer2D::DrawQuad({x, winH * 0.5f}, {2.0f, winH}, {1.0f, 1.0f, 1.0f, a});
    }
    for (float y = 0.0f; y <= winH; y += step) {
        float a = ((int)y % 100 == 0) ? 0.15f : 0.08f;
        Graphic::Renderer2D::DrawQuad({winW * 0.5f, y}, {winW, 2.0f}, {1.0f, 1.0f, 1.0f, a});
    }

    // ── 绘制场景文件中的 SpriteRenderer（静态基础） ──
    uint32_t sceneSpriteCount = 0;
    for (auto& go : scene->GetGameObjects()) {
        auto sr = go->GetComponent<Graphic::SpriteRenderer>();
        if (!sr) continue;
        ++sceneSpriteCount;

        float cx = sr->GetPosition().x + sr->GetSize().x * 0.5f;
        float cy = sr->GetPosition().y + sr->GetSize().y * 0.5f;
        Matrix4 t = glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, 0.0f));
        if (std::abs(sr->GetRotation()) > 0.001f)
            t = glm::rotate(t, glm::radians(sr->GetRotation()), glm::vec3(0, 0, 1));
        t = glm::scale(t, glm::vec3(sr->GetSize(), 1.0f));
        Graphic::Renderer2D::DrawQuad(t, sr->GetColor());
    }

    // ── 绘制 C# 脚本创建的实体（动态层） ──
    auto& scriptEngine = Engine::Get().GetScriptEngine();
    uint32_t scriptEntityCount = 0;
    if (scriptEngine.IsInitialized()) {
        auto* rb = scriptEngine.GetRenderBuffer();
        auto* tb = scriptEngine.GetCurrentTransformBuffer();
        
        for (uint32_t i = 0; i < Scripting::kMaxEntities; ++i) {
            if (!rb->active[i]) continue;
            ++scriptEntityCount;

            float cx = tb->posX[i] + rb->sizeW[i] * 0.5f;
            float cy = tb->posY[i] + rb->sizeH[i] * 0.5f;

            Matrix4 t = glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, 0.0f));
            if (std::abs(tb->rotation[i]) > 0.001f)
                t = glm::rotate(t, tb->rotation[i], glm::vec3(0, 0, 1));
            t = glm::scale(t, glm::vec3(rb->sizeW[i], rb->sizeH[i], 1.0f));
            
            Color color = {rb->colorR[i], rb->colorG[i], rb->colorB[i], rb->colorA[i]};
            Graphic::Renderer2D::DrawQuad(t, color);

            std::string coord = "(" + std::to_string((int)tb->posX[i]) + "," + std::to_string((int)tb->posY[i]) + ")";
            Color inv = {1.0f - rb->colorR[i], 1.0f - rb->colorG[i], 1.0f - rb->colorB[i], 1.0f};
            // 投影线从精灵中心到坐标轴
            Graphic::Renderer2D::DrawQuad({cx, cy * 0.5f}, {1.5f, cy}, inv);
            Graphic::Renderer2D::DrawQuad({cx * 0.5f, cy}, {cx, 1.5f}, inv);
            Graphic::Renderer2D::DrawString(coord, {cx + 10.0f, cy + 10.0f}, 2.0f, inv);
        }
    }

    // HUD 显示两种来源的计数
    uint32_t totalDrawn = sceneSpriteCount + scriptEntityCount;

    // 坐标轴
    Graphic::Renderer2D::DrawQuad({winW * 0.5f, 0.0f}, {winW, 3.0f}, {1.0f, 0.2f, 0.2f, 0.9f});
    Graphic::Renderer2D::DrawQuad({0.0f, winH * 0.5f}, {3.0f, winH}, {0.2f, 1.0f, 0.2f, 0.9f});
    Graphic::Renderer2D::DrawString("X", {winW - 50.0f, 15.0f}, 3.0f, {1.0f, 0.2f, 0.2f, 1.0f});
    Graphic::Renderer2D::DrawString("Y", {15.0f, winH - 50.0f}, 3.0f, {0.2f, 1.0f, 0.2f, 1.0f});

    // ── HUD 覆盖层 ──
    {
        Vector3 camPos = ortho->GetPosition();
        auto hud = [&](float sx, float sy) { return Vector2{sx + camPos.x, sy + camPos.y}; };

        static std::string timingInfo = "Calculating...";
        static std::string pInf = "Loading...";
        static std::string resInfo = "";
        static std::string dcInfo = "";
        static Prisma::Color pC = {0.2f, 1.0f, 0.2f, 1.0f};
        static float pT = 0.0f;
        static double lastT = 0.0;
        double nowT = Platform::GetTimeSeconds();
        float dt = (lastT > 0) ? (float)(nowT - lastT) : 0.016f;
        lastT = nowT;

        const auto& st = Engine::Get().GetFrameStats();
        pT += dt;
        if (pT >= 1.0f) {
            char buf[256];
            snprintf(buf, sizeof(buf), "BF=%.2f Render=%.2f EF=%.2f Present=%.2f Total=%.2f (ms)",
                     st.BeginFrameTime, st.RenderTime, st.EndFrameTime, st.PresentTime, st.TotalTime);
            timingInfo = buf;
            double mt = st.BeginFrameTime;
            std::string ms = "BF";
            if (st.RenderTime > mt) { mt = st.RenderTime; ms = "Render(CPU)"; }
            if (st.EndFrameTime > mt) { mt = st.EndFrameTime; ms = "EF(GPU)"; }
            if (st.PresentTime > mt) { mt = st.PresentTime; ms = "Present"; }
            if (st.TotalTime < 2.0) {
                pInf = "Status: Balanced (Lead: " + ms + ")"; pC = {0.2f, 1.0f, 0.2f, 1.0f};
            } else {
                pInf = "Status: LEAD " + ms;
                pC = (ms.find("CPU") != std::string::npos) ? Prisma::Color{1.0f, 0.2f, 0.8f, 1.0f} : Prisma::Color{1.0f, 0.2f, 0.2f, 1.0f};
            }
            std::string batchStatus = Graphic::Renderer2D::IsBatchingEnabled() ? "ON" : "OFF";
            resInfo = std::to_string(m_Spec.Width) + "x" + std::to_string(m_Spec.Height) + " @ " + std::to_string((int)Engine::Get().GetFPS()) + " FPS (Batch: " + batchStatus + ")";
            auto rs = Graphic::Renderer2D::GetStats();
            int sC = (int)rs.QuadCount - (int)rs.DrawCalls;
            int sP = (rs.QuadCount > 0) ? (int)((float)sC / (float)rs.QuadCount * 100.0f) : 0;
            dcInfo = "DC: " + std::to_string(rs.DrawCalls) + " | Quads: " + std::to_string(rs.QuadCount) + " (-" + std::to_string(sC) + " DCs, Saved: " + std::to_string(sP) + "%)";
            pT = 0.0f;
        }

        Graphic::Renderer2D::DrawString(timingInfo, hud(130.0f, winH - 45.0f), 1.5f, {0.2f, 1.0f, 0.2f, 1.0f});
        Graphic::Renderer2D::DrawString(pInf, hud(130.0f, winH - 85.0f), 1.5f, pC);
        float resW = Graphic::Renderer2D::GetStringWidth(resInfo, 3.0f);
        Graphic::Renderer2D::DrawString(resInfo, hud(winW - resW - 30.0f, winH - 50.0f), 3.0f, {0.4f, 0.7f, 0.4f, 1.0f});
        std::string gpuName = Engine::Get().GetGPUName();
        if (!gpuName.empty()) {
            float gW = Graphic::Renderer2D::GetStringWidth(gpuName, 2.0f);
            Graphic::Renderer2D::DrawString(gpuName, hud(winW - gW - 30.0f, winH - 95.0f), 2.0f, {0.5f, 0.5f, 0.5f, 1.0f});
        }
        float dW = Graphic::Renderer2D::GetStringWidth(dcInfo, 2.0f);
        Graphic::Renderer2D::DrawString(dcInfo, hud(winW - dW - 30.0f, winH - 135.0f), 2.0f, {0.5f, 0.7f, 0.5f, 1.0f});
        Graphic::Renderer2D::DrawString("ESC to exit", hud(winW - 220.0f, 30.0f), 2.0f, {0.4f, 0.4f, 0.4f, 1.0f});
        Graphic::Renderer2D::DrawString("Template2D (" + std::to_string(sceneSpriteCount) + " scene + " + std::to_string(scriptEntityCount) + " script entities)",
                                        hud(30.0f, 30.0f), 2.0f, {0.6f, 0.6f, 0.6f, 1.0f});
        Graphic::Renderer2D::DrawString("Cam: (" + std::to_string((int)camX) + "," + std::to_string((int)camY) + ")",
                                        hud(30.0f, 65.0f), 1.5f, {0.6f, 0.6f, 0.9f, 1.0f});
    }

    Graphic::Renderer2D::EndScene();
}

void Template2DApp::OnEvent(Event& e) {
    // 让基类处理输入事件（传递给 InputManager）
    Application::OnEvent(e);

    EventDispatcher d(e);
    d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        if (ev.GetKeyCode() == SDL_SCANCODE_ESCAPE) { Close(); return true; }
        if (ev.GetKeyCode() == SDL_SCANCODE_B && !ev.IsRepeat()) {
            bool en = Graphic::Renderer2D::IsBatchingEnabled();
            Graphic::Renderer2D::SetBatchingEnabled(!en);
            return true;
        }
        return false;
    });
}

} // namespace Prisma
