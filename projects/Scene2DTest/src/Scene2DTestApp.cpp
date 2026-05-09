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

    // 获取 GPU 名称
    auto renderSystem = Engine::Get().GetRenderSystem();
    if (renderSystem && renderSystem->GetDevice()) {
        m_gpuName = renderSystem->GetDevice()->GetGPUName();
        LOG_INFO("Scene2DTest", "GPU: {0}", m_gpuName);
    }

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

    // ---- 绘制网格背景（自适应窗口尺寸） ----
    float winW = static_cast<float>(m_Spec.Width);
    float winH = static_cast<float>(m_Spec.Height);
    for (float x = 0; x < winW; x += 40.0f) {
        float alpha = (static_cast<int>(x) % 80 == 0) ? 0.15f : 0.08f;
        Graphic::Renderer2D::DrawQuad(
            {x + 1.0f, winH * 0.5f},
            {2.0f, winH},
            {1.0f, 1.0f, 1.0f, alpha}
        );
    }
    for (float y = 0; y < winH; y += 40.0f) {
        float alpha = (static_cast<int>(y) % 80 == 0) ? 0.15f : 0.08f;
        Graphic::Renderer2D::DrawQuad(
            {winW * 0.5f, y + 1.0f},
            {winW, 2.0f},
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

    // ---- 坐标轴 (左下角原点, Y 向上) ----
    // X 轴: 红色, 沿底部
    Graphic::Renderer2D::DrawQuad(
        {winW * 0.5f, 0.0f}, {winW, 3.0f}, {1.0f, 0.2f, 0.2f, 0.9f}
    );
    // Y 轴: 绿色, 沿左边
    Graphic::Renderer2D::DrawQuad(
        {0.0f, winH * 0.5f}, {3.0f, winH}, {0.2f, 1.0f, 0.2f, 0.9f}
    );
    // 轴标签
    Graphic::Renderer2D::DrawString("X", {winW - 24.0f, 4.0f}, 1.0f, {1.0f, 0.2f, 0.2f, 1.0f});
    Graphic::Renderer2D::DrawString("Y", {4.0f, winH - 24.0f}, 1.0f, {0.2f, 1.0f, 0.2f, 1.0f});
    // 原点标记
    Graphic::Renderer2D::DrawQuad({0.0f, 0.0f}, {6.0f, 6.0f}, {1.0f, 1.0f, 1.0f, 0.8f});

    // ---- 从精灵中心到坐标轴的投影线 + 坐标标注 ----
    for (const auto& sprite : m_sprites) {
        float sx = sprite.position.x, sy = sprite.position.y;
        // 反色 (inverse): 1-r, 1-g, 1-b, 不透明
        Color invColor = {1.0f - sprite.color.r, 1.0f - sprite.color.g, 1.0f - sprite.color.b, 1.0f};
        // 虚线: 每段 20px, 间距 8px (减少 draw call 数量)
        const float dashLen = 20.0f, dashGap = 8.0f, dashStep = dashLen + dashGap;
        for (float y = sy; y > 0.0f; y -= dashStep) {
            float len = (std::min)(dashLen, y);
            Graphic::Renderer2D::DrawQuad({sx, y - len * 0.5f}, {2.0f, len}, invColor);
        }
        for (float x = sx; x > 0.0f; x -= dashStep) {
            float len = (std::min)(dashLen, x);
            Graphic::Renderer2D::DrawQuad({x - len * 0.5f, sy}, {len, 2.0f}, invColor);
        }
        // X 轴刻度标记
        Graphic::Renderer2D::DrawQuad(
            {sx, 0.0f}, {4.0f, 8.0f}, invColor
        );
        // Y 轴刻度标记
        Graphic::Renderer2D::DrawQuad(
            {0.0f, sy}, {8.0f, 4.0f}, invColor
        );
        // 坐标 label (反色)
        std::string coordStr = "(" + std::to_string(static_cast<int>(sx))
                             + ", " + std::to_string(static_cast<int>(sy)) + ")";
        Graphic::Renderer2D::DrawString(coordStr, {sx + 4.0f, sy + 4.0f}, 0.8f, invColor);
    }

    // ---- 绘制 FPS 和说明文字 ----
    std::string fpsText = "FPS: " + std::to_string(static_cast<int>(m_currentFps));
    Graphic::Renderer2D::DrawString(fpsText, {10.0f, 10.0f}, 1.0f, {0.0f, 1.0f, 0.0f, 1.0f});

    float infoY = winH - 30.0f;
    std::string infoText = "2D Scene Test Template - " + std::to_string(m_sprites.size()) + " sprites";
    Graphic::Renderer2D::DrawString(infoText, {10.0f, infoY}, 1.0f, {0.6f, 0.6f, 0.6f, 1.0f});

    // 右下角：渲染分辨率 + FPS
    std::string resText = std::to_string(m_Spec.Width) + "x" + std::to_string(m_Spec.Height)
                        + " @ " + std::to_string(static_cast<int>(m_currentFps)) + " FPS";
    Graphic::Renderer2D::DrawString(resText, {winW - 180.0f, infoY - 15.0f}, 1.0f, {0.4f, 0.7f, 0.4f, 1.0f});

    // GPU 信息
    if (!m_gpuName.empty()) {
        Graphic::Renderer2D::DrawString(m_gpuName, {winW - 280.0f, infoY - 30.0f}, 0.8f, {0.5f, 0.5f, 0.5f, 1.0f});
    }

    // 实时 DrawCall 统计 (QuadCount = GPU DrawIndexed 调用次数)
    auto renderStats = Graphic::Renderer2D::GetStats();
    std::string dcText = "DC: " + std::to_string(renderStats.QuadCount);
    Graphic::Renderer2D::DrawString(dcText, {winW - 120.0f, infoY - 30.0f}, 0.8f, {0.5f, 0.7f, 0.5f, 1.0f});

    Graphic::Renderer2D::DrawString("ESC to exit", {winW - 100.0f, infoY}, 1.0f, {0.4f, 0.4f, 0.4f, 1.0f});

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
            int actualW = 0, actualH = 0;
            SDL_GetWindowSize(window.m_Window, &actualW, &actualH);
            LOG_INFO("Scene2DTest", "F11: 请求 {0}x{1}, SDL 实际 {2}x{3}", newW, newH, actualW, actualH);
            return true;
        }
        return false;
    });

    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        uint32_t w = ev.GetWidth();
        uint32_t h = ev.GetHeight();
        LOG_INFO("Scene2DTest", "收到 WindowResizeEvent: {0}x{1}", w, h);
        m_Spec.Width = w;
        m_Spec.Height = h;
        if (m_camera) {
            m_camera->SetProjection(0.0f, static_cast<float>(w), 0.0f, static_cast<float>(h));
            LOG_INFO("Scene2DTest", "相机投影更新为: 0-{0} x 0-{1}", w, h);
        }
        return false;
    });
}

} // namespace Prisma
