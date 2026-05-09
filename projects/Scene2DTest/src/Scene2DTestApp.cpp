#include "Scene2DTestApp.h"
#include "graphic/Renderer2D.h"
#include "graphic/Renderer.h"
#include "graphic/OrthographicCamera.h"
#include "app/Engine.h"
#include "core/Event.h"
#include "Logger.h"
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_video.h>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>

namespace Prisma {

Scene2DTestApp::Scene2DTestApp()
    : Application({"2D Scene Test", 800, 600})
{
    // 初始化测试场景中的精灵
    m_sprites = {
        {{200.0f, 200.0f}, {100.0f, 100.0f}, {1.0f, 0.2f, 0.2f, 1.0f}, 0.0f, 45.0f},   // 红色 - 旋转
        {{400.0f, 300.0f}, {120.0f, 80.0f},  {0.2f, 1.0f, 0.2f, 1.0f}, 0.0f, -60.0f},  // 绿色 - 反向旋转
        {{600.0f, 450.0f}, {90.0f, 90.0f},   {0.2f, 0.4f, 1.0f, 1.0f}, 0.0f, 30.0f},   // 蓝色 - 慢速旋转
        {{100.0f, 500.0f}, {60.0f, 60.0f},   {1.0f, 1.0f, 0.2f, 1.0f}, 0.0f, 90.0f},   // 黄色 - 快速旋转
        {{700.0f, 100.0f}, {80.0f, 80.0f},   {1.0f, 0.4f, 0.8f, 1.0f}, 0.0f, -120.0f}, // 粉色 - 最快旋转
    };
}

int Scene2DTestApp::OnInitialize() {
    LOG_INFO("Scene2DTest", "正在初始化 2D 场景测试...");

    // 创建正交相机 (800x600 视口)
    m_camera = std::make_shared<Graphic::OrthographicCamera>();
    m_camera->SetProjection(0.0f, 800.0f, 0.0f, 600.0f);

    LOG_INFO("Scene2DTest", "2D 场景测试初始化完成 (800x600, {} 个精灵)", m_sprites.size());
    return 0;
}

void Scene2DTestApp::OnUpdate(Timestep ts) {
    m_totalTime += ts;

    // FPS 计算
    m_frameCount++;
    m_fpsTimer += ts;
    if (m_fpsTimer >= 0.5f) {
        m_currentFps = m_frameCount / m_fpsTimer;
        m_frameCount = 0;
        m_fpsTimer = 0.0f;
    }

    // 更新精灵旋转
    for (auto& sprite : m_sprites) {
        sprite.rotation += sprite.rotationSpeed * ts;
        if (sprite.rotation > 360.0f) sprite.rotation -= 360.0f;
    }
}

void Scene2DTestApp::OnRender() {
    if (!m_camera) return;

    // ========== 开始 2D 场景渲染 ==========
    Graphic::Renderer2D::BeginScene(*m_camera);

    // ---- 绘制网格背景 ----
    for (int x = 0; x < 800; x += 40) {
        float alpha = (x % 80 == 0) ? 0.15f : 0.08f;
        Graphic::Renderer2D::DrawQuad(
            {static_cast<float>(x) + 1.0f, 300.0f},
            {2.0f, 600.0f},
            {1.0f, 1.0f, 1.0f, alpha}
        );
    }
    for (int y = 0; y < 600; y += 40) {
        float alpha = (y % 80 == 0) ? 0.15f : 0.08f;
        Graphic::Renderer2D::DrawQuad(
            {400.0f, static_cast<float>(y) + 1.0f},
            {800.0f, 2.0f},
            {1.0f, 1.0f, 1.0f, alpha}
        );
    }

    // ---- 绘制精灵 ----
    for (const auto& sprite : m_sprites) {
        Vector2 center = sprite.position;
        Matrix4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(center, 0.0f));
        transform = glm::rotate(transform, glm::radians(sprite.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        transform = glm::scale(transform, glm::vec3(sprite.size, 1.0f));
        Graphic::Renderer2D::DrawQuad(transform, sprite.color);
    }

    // 中心十字高亮
    Graphic::Renderer2D::DrawQuad(
        {400.0f, 300.0f},
        {4.0f, 4.0f},
        {1.0f, 1.0f, 1.0f, 0.5f}
    );

    // ---- 绘制 FPS 和说明文字 ----
    std::string fpsText = "FPS: " + std::to_string(static_cast<int>(m_currentFps));
    Graphic::Renderer2D::DrawString(fpsText, {10.0f, 10.0f}, 1.0f, {0.0f, 1.0f, 0.0f, 1.0f});

    std::string infoText = "2D Scene Test Template - " + std::to_string(m_sprites.size()) + " sprites";
    Graphic::Renderer2D::DrawString(infoText, {10.0f, 570.0f}, 1.0f, {0.6f, 0.6f, 0.6f, 1.0f});

    // 右下角：渲染分辨率 + FPS
    std::string resText = std::to_string(m_Spec.Width) + "x" + std::to_string(m_Spec.Height)
                        + " @ " + std::to_string(static_cast<int>(m_currentFps)) + " FPS";
    Graphic::Renderer2D::DrawString(resText, {620.0f, 555.0f}, 1.0f, {0.4f, 0.7f, 0.4f, 1.0f});

    Graphic::Renderer2D::DrawString("ESC to exit", {700.0f, 570.0f}, 1.0f, {0.4f, 0.4f, 0.4f, 1.0f});

    // ========== 结束 2D 场景渲染 ==========
    Graphic::Renderer2D::EndScene();
}

void Scene2DTestApp::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);

    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        if (ev.GetKeyCode() == SDL_SCANCODE_ESCAPE) {
            Close();
            return true;
        }
        if (ev.GetKeyCode() == SDL_SCANCODE_F11 && !ev.IsRepeat()) {
            // 切换分辨率 800x600 ↔ 1280x720
            auto& window = Engine::Get().GetWindow();
            uint32_t newW = (window.GetWidth() == 800) ? 1280 : 800;
            uint32_t newH = (window.GetHeight() == 600) ? 720 : 600;
            SDL_SetWindowSize(window.m_Window, static_cast<int>(newW), static_cast<int>(newH));
            LOG_INFO("Scene2DTest", "F11: 切换分辨率至 {0}x{1}", newW, newH);
            return true;
        }
        return false;
    });

    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        uint32_t w = ev.GetWidth();
        uint32_t h = ev.GetHeight();
        m_Spec.Width = w;
        m_Spec.Height = h;
        if (m_camera) {
            m_camera->SetProjection(0.0f, static_cast<float>(w), 0.0f, static_cast<float>(h));
        }
        return false;
    });
}

} // namespace Prisma
