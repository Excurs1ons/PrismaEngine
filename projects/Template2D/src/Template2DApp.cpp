#include "Template2DApp.h"
#include "graphic/Renderer2D.h"
#include "graphic/Renderer.h"
#include "graphic/OrthographicCamera.h"
#include "app/Engine.h"
#include "core/AssetManager.h"
#include "core/Event.h"
#include "Logger.h"
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_mouse.h>
#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace Prisma {

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
    std::vector<std::string> pPs = { filePath, "assets/project.json", "../projects/Template2D/assets/project.json", "projects/Template2D/assets/project.json" };
    std::ifstream file;
    for (const auto& p : pPs) { file.open(p); if (file.is_open()) { LOG_INFO("Template2D", "已在路径找到项目配置: {0}", p); break; } file.close(); }
    if (!file.is_open()) { LOG_WARNING("Template2D", "未找到项目配置文件，使用内置默认设置 (1920x1080)"); return spec; }
    try {
        nlohmann::json data; file >> data;
        spec.Name = data.value("name", spec.Name);
        spec.EntryScene = data.value("entryScene", spec.EntryScene);
        if (data.contains("window")) {
            auto& w = data["window"];
            spec.Width = w.value("width", spec.Width); spec.Height = w.value("height", spec.Height);
            spec.Fullscreen = w.value("fullscreen", spec.Fullscreen); spec.Resizable = w.value("resizable", spec.Resizable);
            std::string vs = w.value("vsync", "VSync");
            if (vs == "Immediate") spec.PresentMode = Graphic::PresentMode::Immediate;
            else if (vs == "Mailbox") spec.PresentMode = Graphic::PresentMode::Mailbox;
            else if (vs == "Adaptive") spec.PresentMode = Graphic::PresentMode::Adaptive;
            else spec.PresentMode = Graphic::PresentMode::VSync;
            spec.MaxFPS = w.value("maxFPS", spec.MaxFPS);
        }
    } catch (const std::exception& e) { LOG_ERROR("Template2D", "解析项目配置失败: {0}", e.what()); }
    return spec;
}

int Template2DApp::OnInitialize() {
    auto assetManager = Engine::Get().GetAssetManager();
    if (assetManager) { assetManager->AddSearchPath("assets"); assetManager->AddSearchPath("projects/Template2D/assets"); }
    auto& window = Engine::Get().GetWindow();
    m_Spec.Width = window.GetWidth(); m_Spec.Height = window.GetHeight();
    
    // 初始化相机组件 (替代之前的直接创建相机)
    m_CameraComponent = std::make_shared<CameraComponent>();
    
    std::string sceneToLoad = m_Spec.EntryScene.empty() ? "scenes/2d_test.json" : m_Spec.EntryScene;
    LoadScene(sceneToLoad);
    
    LOG_INFO("Template2D", "正在生成 20 个额外的动态对象...");
    for (int i = 0; i < 20; ++i) {
        TestSprite s; s.position = { (float)(rand() % m_Spec.Width), (float)(rand() % m_Spec.Height) };
        s.size = { (float)(40 + rand() % 60), (float)(40 + rand() % 60) };
        s.color = { (float)(rand() % 100) / 100.0f, (float)(rand() % 100) / 100.0f, (float)(rand() % 100) / 100.0f, 1.0f };
        s.rotation = (float)(rand() % 360); s.rotationSpeed = (float)(30 + rand() % 120) * ((rand() % 2 == 0) ? 1.0f : -1.0f);
        m_sprites.push_back(s);
    }
    
    // 强制使相机投影匹配窗口分辨率
    m_CameraComponent->SetProjection(0.0f, (float)m_Spec.Width, 0.0f, (float)m_Spec.Height);
    LOG_INFO("Template2D", "初始化完成: 分辨率={0}x{1}, 投影=0-{0}x0-{1}", m_Spec.Width, m_Spec.Height);
    
    auto renderSystem = Engine::Get().GetRenderSystem();
    if (renderSystem && renderSystem->GetDevice()) m_gpuName = renderSystem->GetDevice()->GetGPUName();
    return 0;
}

void Template2DApp::OnUpdate(Timestep ts) {
    m_totalTime += ts; m_frameCount++; m_fpsTimer += ts;
    if (m_fpsTimer >= 1.0f) { m_currentFps = m_frameCount / m_fpsTimer; m_frameCount = 0; m_fpsTimer = 0.0f; }
    for (auto& s : m_sprites) { s.rotation += s.rotationSpeed * ts; if (s.rotation > 360.0f) s.rotation -= 360.0f; }
}

void Template2DApp::OnRender() {
    if (!m_CameraComponent) return;
    
    // 使用组件内部的 ICamera 进行 2D 场景绘制
    auto camera = std::dynamic_pointer_cast<Graphic::OrthographicCamera>(m_CameraComponent->GetCamera());
    if (camera) {
        Graphic::Renderer2D::BeginScene(*camera);
    } else {
        return;
    }

    float winW = (float)m_Spec.Width, winH = (float)m_Spec.Height;
    const float step = 50.0f;
    for (float x = 0.0f; x <= winW; x += step) { float a = ((int)x % 100 == 0) ? 0.15f : 0.08f; Graphic::Renderer2D::DrawQuad({x, winH * 0.5f}, {2.0f, winH}, {1.0f, 1.0f, 1.0f, a}); }
    for (float y = 0.0f; y <= winH; y += step) { float a = ((int)y % 100 == 0) ? 0.15f : 0.08f; Graphic::Renderer2D::DrawQuad({winW * 0.5f, y}, {winW, 2.0f}, {1.0f, 1.0f, 1.0f, a}); }
    for (const auto& s : m_sprites) {
        Matrix4 t = glm::translate(glm::mat4(1.0f), glm::vec3(s.position, 0.0f));
        t = glm::rotate(t, glm::radians(s.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        t = glm::scale(t, glm::vec3(s.size, 1.0f));
        Graphic::Renderer2D::DrawQuad(t, s.color);
    }

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

    static std::string pInf = "Loading..."; static Prisma::Color pC = {0.2f, 1.0f, 0.2f, 1.0f}; static float pT = 0.0f;
    const auto& st = Engine::Get().GetFrameStats(); pT += dt;
    if (pT >= 1.0f) {
        char buf[256]; snprintf(buf, sizeof(buf), "BF=%.2f Render=%.2f EF=%.2f Present=%.2f Total=%.2f (ms)", st.BeginFrameTime, st.RenderTime, st.EndFrameTime, st.PresentTime, st.TotalTime);
        double mt = st.BeginFrameTime; std::string ms = "BF";
        if (st.RenderTime > mt) { mt = st.RenderTime; ms = "Render(CPU-Collect)"; }
        if (st.EndFrameTime > mt) { mt = st.EndFrameTime; ms = "EF(GPU-Driver)"; }
        if (st.PresentTime > mt) { mt = st.PresentTime; ms = "Present"; }
        if (st.TotalTime < 2.0) { pInf = std::string(buf) + " | Status: Balanced (Lead: " + ms + ")"; pC = {0.2f, 1.0f, 0.2f, 1.0f}; }
        else { pInf = std::string(buf) + " | Status: LEAD " + ms; pC = (ms.find("CPU") != std::string::npos) ? Prisma::Color{1.0f, 0.2f, 0.8f, 1.0f} : Prisma::Color{1.0f, 0.2f, 0.2f, 1.0f}; }
        pT = 0.0f;
    }
    Graphic::Renderer2D::DrawString(pInf, {100.0f, winH - 45.0f}, 1.5f, pC);

    std::string batchStatus = Graphic::Renderer2D::IsBatchingEnabled() ? "ON" : "OFF";
    std::string resT = std::to_string(m_Spec.Width) + "x" + std::to_string(m_Spec.Height) + " @ " + std::to_string((int)m_currentFps) + " FPS (Batch: " + batchStatus + ")";
    float resW = Graphic::Renderer2D::GetStringWidth(resT, 3.0f);
    Graphic::Renderer2D::DrawString(resT, {winW - resW - 30.0f, winH - 50.0f}, 3.0f, {0.4f, 0.7f, 0.4f, 1.0f});
    if (!m_gpuName.empty()) { float gW = Graphic::Renderer2D::GetStringWidth(m_gpuName, 2.0f); Graphic::Renderer2D::DrawString(m_gpuName, {winW - gW - 30.0f, winH - 95.0f}, 2.0f, {0.5f, 0.5f, 0.5f, 1.0f}); }
    auto rs = Graphic::Renderer2D::GetStats();
    int sC = (int)rs.QuadCount - (int)rs.DrawCalls; int sP = (rs.QuadCount > 0) ? (int)((float)sC / (float)rs.QuadCount * 100.0f) : 0;
    std::string dcS = "DC: " + std::to_string(rs.DrawCalls) + " | Quads: " + std::to_string(rs.QuadCount) + " (-" + std::to_string(sC) + " DCs, Saved: " + std::to_string(sP) + "%)";
    float dW = Graphic::Renderer2D::GetStringWidth(dcS, 2.0f);
    Graphic::Renderer2D::DrawString(dcS, {winW - dW - 30.0f, winH - 135.0f}, 2.0f, {0.5f, 0.7f, 0.5f, 1.0f});
    Graphic::Renderer2D::DrawString("ESC to exit", {winW - 220.0f, 30.0f}, 2.0f, {0.4f, 0.4f, 0.4f, 1.0f});
    Graphic::Renderer2D::DrawString("Template2D - " + std::to_string(m_sprites.size()) + " sprites", {30.0f, 30.0f}, 2.0f, {0.6f, 0.6f, 0.6f, 1.0f});
    
    Graphic::Renderer2D::EndScene();
}

void Template2DApp::OnEvent(Event& e) {
    EventDispatcher d(e);
    d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) { 
        if (ev.GetKeyCode() == SDL_SCANCODE_ESCAPE) { Close(); return true; } 
        if (ev.GetKeyCode() == SDL_SCANCODE_B && !ev.IsRepeat()) { bool en = Graphic::Renderer2D::IsBatchingEnabled(); Graphic::Renderer2D::SetBatchingEnabled(!en); return true; }
        return false; 
    });
    d.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) { uint32_t w = ev.GetWidth(), h = ev.GetHeight(); static uint32_t lW = 0, lH = 0; if (w == lW && h == lH) return false; lW = w; lH = h; m_Spec.Width = w; m_Spec.Height = h; if (m_CameraComponent) m_CameraComponent->SetProjection(0.0f, (float)w, 0.0f, (float)h); return false; });
}

bool Template2DApp::LoadScene(const std::string& fP) {
    std::string aP = fP; auto aM = Engine::Get().GetAssetManager(); if (aM) { auto f = aM->FindResource(fP); if (f) aP = f->string(); }
    std::ifstream file(aP); if (!file.is_open()) return false;
    try {
        nlohmann::json d; file >> d;
        if (d.contains("camera") && m_CameraComponent) { auto& c = d["camera"]; if (c.contains("projection")) { auto& p = c["projection"]; m_CameraComponent->SetProjection(p.value("left", 0.0f), p.value("right", 1920.0f), p.value("bottom", 0.0f), p.value("top", 1080.0f)); } }
        if (d.contains("sprites") && d["sprites"].is_array()) { m_sprites.clear(); for (const auto& s : d["sprites"]) { TestSprite sp; if (s.contains("position")) sp.position = {s["position"][0].get<float>(), s["position"][1].get<float>()}; if (s.contains("size")) sp.size = {s["size"][0].get<float>(), s["size"][1].get<float>()}; if (s.contains("color")) sp.color = {s["color"][0].get<float>(), s["color"][1].get<float>(), s["color"][2].get<float>(), s["color"][3].get<float>()}; sp.rotation = s.value("rotation", 0.0f); sp.rotationSpeed = s.value("rotationSpeed", 0.0f); m_sprites.push_back(sp); } return true; }
    } catch (...) {}
    return false;
}

} // namespace Prisma
