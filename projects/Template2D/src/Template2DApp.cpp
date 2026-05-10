#include "Template2DApp.h"
#include "graphic/Renderer2D.h"
#include "graphic/Renderer.h"
#include "graphic/OrthographicCamera.h"
#include "app/Engine.h"
#include "SceneManager.h"
#include "core/Event.h"
#include "Logger.h"
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_mouse.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glaze/glaze.hpp>
#include <compare>
#include <vector>
#include <array>
#include <string>

// ============================================================================
// 1. 定义 JSON 数据结构 (Glaze 声明式映射)
// ============================================================================

namespace Prisma {

// --- Project Config 结构 ---
struct WindowConfig {
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool fullscreen = true;
    bool resizable = true;
    Graphic::PresentMode vsync = Graphic::PresentMode::Mailbox;
    uint32_t maxFPS = 0;
};

struct ProjectConfig {
    std::string name = "Template2D";
    std::string entryScene = "scenes/2d_test.jsonc";
    std::vector<std::string> assets;
    WindowConfig window;
};

} // namespace Prisma

// --- Glaze 映射 ---
template <>
struct glz::meta<Prisma::Graphic::PresentMode> {
    using enum Prisma::Graphic::PresentMode;
    static constexpr auto value = glz::enumerate(
        "Immediate", Immediate,
        "Mailbox", Mailbox,
        "Adaptive", Adaptive,
        "VSync", VSync
    );
};

template <>
struct glz::meta<Prisma::WindowConfig> {
    static constexpr auto value = glz::object(
        "width", &Prisma::WindowConfig::width,
        "height", &Prisma::WindowConfig::height,
        "fullscreen", &Prisma::WindowConfig::fullscreen,
        "resizable", &Prisma::WindowConfig::resizable,
        "vsync", &Prisma::WindowConfig::vsync,
        "maxFPS", &Prisma::WindowConfig::maxFPS
    );
};

template <>
struct glz::meta<Prisma::ProjectConfig> {
    static constexpr auto value = glz::object(
        "name", &Prisma::ProjectConfig::name,
        "entryScene", &Prisma::ProjectConfig::entryScene,
        "assets", &Prisma::ProjectConfig::assets,
        "window", &Prisma::ProjectConfig::window
    );
};

namespace Prisma {

// ============================================================================
// 2. Template2DApp 实现
// ============================================================================

Template2DApp::Template2DApp()
    : Template2DApp(LoadSpecification("projects/Template2D/assets/project.json"))
{
}

Template2DApp::Template2DApp(const ApplicationSpecification& spec)
    : Application(spec)
{
}

ApplicationSpecification Template2DApp::LoadSpecification(const std::string& filePath) {
    ApplicationSpecification spec;
    spec.Name = "Template2D";
    spec.Width = 1920; spec.Height = 1080; spec.Fullscreen = true; spec.Resizable = true;
    spec.PresentMode = Graphic::PresentMode::Mailbox;

    std::vector<std::string> pPs = { filePath, "assets/project.json", "../projects/Template2D/assets/project.json", "projects/Template2D/assets/project.json" };
    std::string finalPath;
    for (const auto& p : pPs) { if (std::filesystem::exists(p)) { finalPath = p; break; } }

    if (finalPath.empty()) {
        LOG_WARNING("Template2D", "未找到项目配置文件，使用内置默认设置 (1920x1080, Mailbox)");
        return spec;
    }

    ProjectConfig config;
    auto error = glz::read_file_json(config, finalPath, std::string{});
    if (error) {
        LOG_ERROR("Template2D", "解析项目配置失败: {0}", glz::format_error(error, ""));
        return spec;
    }

    LOG_INFO("Template2D", "成功加载项目配置: {0}, 呈现模式: {1}", config.name, static_cast<int>(config.window.vsync));

    spec.Name = config.name;
    spec.EntryScene = config.entryScene;
    spec.Width = config.window.width; spec.Height = config.window.height;
    spec.Fullscreen = config.window.fullscreen; spec.Resizable = config.window.resizable;
    spec.PresentMode = config.window.vsync; spec.MaxFPS = config.window.maxFPS;
    return spec;
}

int Template2DApp::OnInitialize() {
    // 引擎自动从 project.json 加载资产路径
    // 引擎 SceneManager 加载场景（相机 + GameObjects）
    std::string sceneToLoad = m_Spec.EntryScene.empty() ? "scenes/2d_test.jsonc" : m_Spec.EntryScene;
    auto sceneManager = Engine::Get().GetSceneManager();
    if (sceneManager) sceneManager->LoadFromFile(sceneToLoad);

    // 缓存场景中的 SpriteRenderer 引用
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (scene) {
        for (auto& go : scene->GetGameObjects()) {
            auto sr = go->GetComponent<Graphic::SpriteRenderer>();
            if (sr) m_sceneSprites.push_back(sr);
        }
    }

    // 生成 20 个额外的动态测试精灵
    LOG_INFO("Template2D", "正在生成 20 个额外的动态对象...");
    for (int i = 0; i < 20; ++i) {
        TestSprite s; 
        s.position = { (float)(rand() % m_Spec.Width), (float)(rand() % m_Spec.Height) };
        s.size = { (float)(40 + rand() % 60), (float)(40 + rand() % 60) };
        s.color = { (float)(rand() % 100) / 100.0f, (float)(rand() % 100) / 100.0f, (float)(rand() % 100) / 100.0f, 1.0f };
        s.rotation = (float)(rand() % 360); s.rotationSpeed = (float)(30 + rand() % 120) * ((rand() % 2 == 0) ? 1.0f : -1.0f);
        m_sprites.push_back(s);
    }

    LOG_INFO("Template2D", "初始化完成: 分辨率={0}x{1}", m_Spec.Width, m_Spec.Height);
    return 0;
}

void Template2DApp::OnUpdate(Timestep ts) {
    // 动态精灵旋转
    for (auto& s : m_sprites) {
        s.rotation += s.rotationSpeed * ts;
        if (s.rotation > 360.0f) s.rotation -= 360.0f;
    }
}

void Template2DApp::OnRender() {
    // 获取相机（来自 SceneManager）
    auto* scene = Engine::Get().GetSceneManager()->GetCurrentScene();
    auto camera = scene ? scene->GetMainCamera() : nullptr;
    auto ortho = std::dynamic_pointer_cast<Graphic::OrthographicCamera>(camera);
    if (!ortho) return;

    Graphic::Renderer2D::BeginScene(*ortho);

    float winW = (float)m_Spec.Width, winH = (float)m_Spec.Height;
    const float step = 50.0f;
    for (float x = 0.0f; x <= winW; x += step) { float a = ((int)x % 100 == 0) ? 0.15f : 0.08f; Graphic::Renderer2D::DrawQuad({x, winH * 0.5f}, {2.0f, winH}, {1.0f, 1.0f, 1.0f, a}); }
    for (float y = 0.0f; y <= winH; y += step) { float a = ((int)y % 100 == 0) ? 0.15f : 0.08f; Graphic::Renderer2D::DrawQuad({winW * 0.5f, y}, {winW, 2.0f}, {1.0f, 1.0f, 1.0f, a}); }

    // 绘制场景精灵（GameObject + SpriteRenderer — 引擎场景系统）
    for (auto& sr : m_sceneSprites) {
        Vector2 center = sr->GetPosition() + sr->GetSize() * 0.5f;
        Matrix4 t = glm::translate(glm::mat4(1.0f), glm::vec3(center, 0.0f));
        if (std::abs(sr->GetRotation()) > 0.001f)
            t = glm::rotate(t, glm::radians(sr->GetRotation()), glm::vec3(0, 0, 1));
        t = glm::scale(t, glm::vec3(sr->GetSize(), 1.0f));
        Graphic::Renderer2D::DrawQuad(t, sr->GetColor());
    }

    // 绘制动态精灵（TestSprite — App 管理，含旋转动画）
    for (const auto& s : m_sprites) {
        Matrix4 t = glm::translate(glm::mat4(1.0f), glm::vec3(s.position, 0.0f));
        t = glm::rotate(t, glm::radians(s.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        t = glm::scale(t, glm::vec3(s.size, 1.0f));
        Graphic::Renderer2D::DrawQuad(t, s.color);
    }

    // 坐标标注
    static std::vector<std::string> coordCache;
    static float coordTimer = 0.0f; static double lastT = 0.0;
    double nowT = Platform::GetTimeSeconds(); float dt = (lastT > 0) ? (float)(nowT - lastT) : 0.016f; lastT = nowT;
    coordTimer += dt;
    if (coordCache.size() != m_sprites.size() || coordTimer >= 1.0f) {
        coordCache.clear();
        for (const auto& s : m_sprites) coordCache.push_back("(" + std::to_string((int)s.position.x) + "," + std::to_string((int)s.position.y) + ")");
        coordTimer = 0.0f;
    }
    int idx = 0;
    for (const auto& s : m_sprites) {
        Color inv = {1.0f - s.color.r, 1.0f - s.color.g, 1.0f - s.color.b, 1.0f};
        Graphic::Renderer2D::DrawQuad({s.position.x, s.position.y * 0.5f}, {1.5f, s.position.y}, inv);
        Graphic::Renderer2D::DrawQuad({s.position.x * 0.5f, s.position.y}, {s.position.x, 1.5f}, inv);
        Graphic::Renderer2D::DrawString(coordCache[idx++], {s.position.x + 10.0f, s.position.y + 10.0f}, 2.0f, inv);
    }

    Graphic::Renderer2D::DrawQuad({winW * 0.5f, 0.0f}, {winW, 3.0f}, {1.0f, 0.2f, 0.2f, 0.9f});
    Graphic::Renderer2D::DrawQuad({0.0f, winH * 0.5f}, {3.0f, winH}, {0.2f, 1.0f, 0.2f, 0.9f});
    Graphic::Renderer2D::DrawString("X", {winW - 50.0f, 15.0f}, 3.0f, {1.0f, 0.2f, 0.2f, 1.0f});
    Graphic::Renderer2D::DrawString("Y", {15.0f, winH - 50.0f}, 3.0f, {0.2f, 1.0f, 0.2f, 1.0f});

    // 性能诊断与状态面板 (由 Engine 提供统计数据)
    static std::string timingInfo = "Calculating..."; 
    static std::string pInf = "Loading..."; 
    static std::string resInfo = ""; 
    static std::string dcInfo = "";
    static Prisma::Color pC = {0.2f, 1.0f, 0.2f, 1.0f}; 
    static float pT = 0.0f;
    
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
            pInf = "Status: Balanced (Lead: " + ms + ")"; 
            pC = {0.2f, 1.0f, 0.2f, 1.0f}; 
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
    
    // 渲染统计
    Graphic::Renderer2D::DrawString(timingInfo, {130.0f, winH - 45.0f}, 1.5f, {0.2f, 1.0f, 0.2f, 1.0f});
    Graphic::Renderer2D::DrawString(pInf, {130.0f, winH - 85.0f}, 1.5f, pC);

    float resW = Graphic::Renderer2D::GetStringWidth(resInfo, 3.0f);
    Graphic::Renderer2D::DrawString(resInfo, {winW - resW - 30.0f, winH - 50.0f}, 3.0f, {0.4f, 0.7f, 0.4f, 1.0f});
    std::string gpuName = Engine::Get().GetGPUName();
    if (!gpuName.empty()) { float gW = Graphic::Renderer2D::GetStringWidth(gpuName, 2.0f); Graphic::Renderer2D::DrawString(gpuName, {winW - gW - 30.0f, winH - 95.0f}, 2.0f, {0.5f, 0.5f, 0.5f, 1.0f}); }
    float dW = Graphic::Renderer2D::GetStringWidth(dcInfo, 2.0f);
    Graphic::Renderer2D::DrawString(dcInfo, {winW - dW - 30.0f, winH - 135.0f}, 2.0f, {0.5f, 0.7f, 0.5f, 1.0f});
    Graphic::Renderer2D::DrawString("ESC to exit", {winW - 220.0f, 30.0f}, 2.0f, {0.4f, 0.4f, 0.4f, 1.0f});
    Graphic::Renderer2D::DrawString("Template2D - " + std::to_string(m_sceneSprites.size() + m_sprites.size()) + " sprites", {30.0f, 30.0f}, 2.0f, {0.6f, 0.6f, 0.6f, 1.0f});
    
    Graphic::Renderer2D::EndScene();
}

void Template2DApp::OnEvent(Event& e) {
    EventDispatcher d(e);
    d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) { 
        if (ev.GetKeyCode() == SDL_SCANCODE_ESCAPE) { Close(); return true; } 
        if (ev.GetKeyCode() == SDL_SCANCODE_B && !ev.IsRepeat()) { bool en = Graphic::Renderer2D::IsBatchingEnabled(); Graphic::Renderer2D::SetBatchingEnabled(!en); return true; }
        return false; 
    });
    d.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) { 
        m_Spec.Width = ev.GetWidth(); m_Spec.Height = ev.GetHeight(); 
        return false; 
    });
}

} // namespace Prisma
