#include "Scene2DTestApp.h"
#include "graphic/Renderer2D.h"
#include "graphic/Renderer.h"
#include "graphic/OrthographicCamera.h"
#include "app/Engine.h"
#include "core/AssetManager.h"
#include "core/Event.h"
#include "Logger.h"
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_video.h>
#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace Prisma {

Scene2DTestApp::Scene2DTestApp()
    : Scene2DTestApp(LoadSpecification("projects/Scene2DTest/assets/project.json"))
{
}

Scene2DTestApp::Scene2DTestApp(const ApplicationSpecification& spec)
    : Application(spec)
{
}

ApplicationSpecification Scene2DTestApp::LoadSpecification(const std::string& filePath) {
    ApplicationSpecification spec;
    spec.Name = "2D Scene Test";
    spec.Width = 800;
    spec.Height = 600;
    spec.Fullscreen = false;
    spec.Resizable = true;

    // 尝试多个可能的路径
    std::vector<std::string> potentialPaths = {
        filePath,                                  // 原始路径
        "assets/project.json",                     // 相对于 build/bin 的路径 (CMake 复制后的位置)
        "../projects/Scene2DTest/assets/project.json" // 相对于 build/bin 运行时的源码位置
    };

    std::ifstream file;
    std::string finalPath;
    for (const auto& path : potentialPaths) {
        file.open(path);
        if (file.is_open()) {
            finalPath = path;
            break;
        }
    }

    if (!file.is_open()) {
        LOG_INFO("Scene2DTest", "未找到项目配置文件，使用默认设置");
        return spec;
    }

    try {
        nlohmann::json data;
        file >> data;

        spec.Name = data.value("name", spec.Name);
        spec.EntryScene = data.value("entryScene", spec.EntryScene);
        if (data.contains("window")) {
            auto& window = data["window"];
            spec.Width = window.value("width", spec.Width);
            spec.Height = window.value("height", spec.Height);
            spec.Fullscreen = window.value("fullscreen", spec.Fullscreen);
            spec.Resizable = window.value("resizable", spec.Resizable);
            
            std::string vsyncMode = window.value("vsync", "VSync");
            if (vsyncMode == "Immediate") spec.PresentMode = Graphic::PresentMode::Immediate;
            else if (vsyncMode == "Mailbox") spec.PresentMode = Graphic::PresentMode::Mailbox;
            else if (vsyncMode == "Adaptive") spec.PresentMode = Graphic::PresentMode::Adaptive;
            else spec.PresentMode = Graphic::PresentMode::VSync;

            spec.MaxFPS = window.value("maxFPS", spec.MaxFPS);

            LOG_INFO("Scene2DTest", "成功加载项目配置: {0}", filePath);
            LOG_INFO("Scene2DTest", "  - 应用名称: {0}", spec.Name);
            LOG_INFO("Scene2DTest", "  - 入口场景: {0}", spec.EntryScene);
            LOG_INFO("Scene2DTest", "  - 窗口尺寸: {0}x{1}", spec.Width, spec.Height);
            LOG_INFO("Scene2DTest", "  - 全屏: {0}", spec.Fullscreen ? "是" : "否");
            LOG_INFO("Scene2DTest", "  - 可调大小: {0}", spec.Resizable ? "是" : "否");
            LOG_INFO("Scene2DTest", "  - VSync 模式: {0}", vsyncMode);
            LOG_INFO("Scene2DTest", "  - 最大 FPS: {0}", spec.MaxFPS);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Scene2DTest", "解析项目配置文件失败: {0}", e.what());
    }

    return spec;
}

int Scene2DTestApp::OnInitialize() {
    LOG_INFO("Scene2DTest", "正在初始化 2D 场景测试...");

    // 添加项目特定的资产搜索路径
    auto assetManager = Engine::Get().GetAssetManager();
    if (assetManager) {
        assetManager->AddSearchPath("assets");                          // 本地构建输出目录
        assetManager->AddSearchPath("projects/Scene2DTest/assets");    // 原始项目目录
    }

    // 创建正交相机
    m_camera = std::make_shared<Graphic::OrthographicCamera>();

    std::string sceneToLoad = m_Spec.EntryScene;
    if (sceneToLoad.empty()) {
        sceneToLoad = "scenes/default.json";
        LOG_INFO("Scene2DTest", "入口场景未配置，尝试加载默认场景: {0}", sceneToLoad);
    }

    bool loaded = LoadScene(sceneToLoad);

    if (!loaded) {
        if (m_Spec.EntryScene.empty()) {
            LOG_ERROR("Scene2DTest", "入口场景未配置且未找到默认场景 'scenes/default.json'，程序退出。");
            return -1;
        }
        
        LOG_INFO("Scene2DTest", "使用硬编码场景数据");
        // 初始化测试场景中的精灵
        m_sprites = {
            {{200.0f, 200.0f}, {100.0f, 100.0f}, {1.0f, 0.2f, 0.2f, 1.0f}, 0.0f, 45.0f},   // 红色 - 旋转
            {{400.0f, 300.0f}, {120.0f, 80.0f},  {0.2f, 1.0f, 0.2f, 1.0f}, 0.0f, -60.0f},  // 绿色 - 反向旋转
            {{600.0f, 450.0f}, {90.0f, 90.0f},   {0.2f, 0.4f, 1.0f, 1.0f}, 0.0f, 30.0f},   // 蓝色 - 慢速旋转
            {{100.0f, 500.0f}, {60.0f, 60.0f},   {1.0f, 1.0f, 0.2f, 1.0f}, 0.0f, 90.0f},   // 黄色 - 快速旋转
            {{700.0f, 100.0f}, {80.0f, 80.0f},   {1.0f, 0.4f, 0.8f, 1.0f}, 0.0f, -120.0f}, // 粉色 - 最快旋转
        };

        // 默认相机配置 (使用应用规格设置的视口)
        m_camera->SetProjection(0.0f, static_cast<float>(m_Spec.Width), 0.0f, static_cast<float>(m_Spec.Height));
    }

    // 获取 GPU 名称
    auto renderSystem = Engine::Get().GetRenderSystem();
    if (renderSystem && renderSystem->GetDevice()) {
        m_gpuName = renderSystem->GetDevice()->GetGPUName();
        LOG_INFO("Scene2DTest", "GPU: {0}", m_gpuName);
    }

    LOG_INFO("Scene2DTest", "2D 场景测试初始化完成 (800x600, {} 个精灵)", m_sprites.size());

    if (m_autoQuit) {
        LOG_INFO("Scene2DTest", "检测到 --quit 参数，正在自动退出...");
        Close();
    }

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

bool Scene2DTestApp::LoadScene(const std::string& filePath) {
    std::string actualPath = filePath;
    auto assetManager = Engine::Get().GetAssetManager();
    if (assetManager) {
        auto foundPath = assetManager->FindResource(filePath);
        if (foundPath) {
            actualPath = foundPath->string();
        }
    }

    std::ifstream file(actualPath);
    if (!file.is_open()) {
        LOG_ERROR("Scene2DTest", "无法打开场景文件: {0} (实际路径: {1})", filePath, actualPath);
        return false;
    }

    try {
        nlohmann::json data;
        file >> data;

        // 加载场景名称 (可选)
        std::string sceneName = data.value("name", "Untitled Scene");
        LOG_INFO("Scene2DTest", "正在从 JSON 加载场景: {0}", sceneName);

        // 加载相机配置
        if (data.contains("camera") && m_camera) {
            auto& cameraJson = data["camera"];
            if (cameraJson.contains("projection")) {
                auto& proj = cameraJson["projection"];
                float left = proj.value("left", 0.0f);
                float right = proj.value("right", 800.0f);
                float bottom = proj.value("bottom", 0.0f);
                float top = proj.value("top", 600.0f);
                m_camera->SetProjection(left, right, bottom, top);
                LOG_INFO("Scene2DTest", "加载相机投影: L={0}, R={1}, B={2}, T={3}", left, right, bottom, top);
            }
        }

        // 加载场景设置
        if (data.contains("settings")) {
            auto& settingsJson = data["settings"];
            if (settingsJson.contains("clearColor") && settingsJson["clearColor"].is_array() && m_camera) {
                auto& cc = settingsJson["clearColor"];
                float r = cc[0].get<float>();
                float g = cc[1].get<float>();
                float b = cc[2].get<float>();
                float a = cc[3].get<float>();
                m_camera->SetClearColor(r, g, b, a);
                LOG_INFO("Scene2DTest", "  - 背景颜色: ({0}, {1}, {2}, {3})", r, g, b, a);
            }
        }

        // 加载精灵
        if (data.contains("sprites") && data["sprites"].is_array()) {
            m_sprites.clear();
            for (const auto& spriteJson : data["sprites"]) {
                TestSprite sprite;
                
                // Position
                if (spriteJson.contains("position") && spriteJson["position"].is_array()) {
                    sprite.position.x = spriteJson["position"][0].get<float>();
                    sprite.position.y = spriteJson["position"][1].get<float>();
                }

                // Size
                if (spriteJson.contains("size") && spriteJson["size"].is_array()) {
                    sprite.size.x = spriteJson["size"][0].get<float>();
                    sprite.size.y = spriteJson["size"][1].get<float>();
                }

                // Color
                if (spriteJson.contains("color") && spriteJson["color"].is_array()) {
                    sprite.color.r = spriteJson["color"][0].get<float>();
                    sprite.color.g = spriteJson["color"][1].get<float>();
                    sprite.color.b = spriteJson["color"][2].get<float>();
                    sprite.color.a = spriteJson["color"][3].get<float>();
                }

                // Rotation
                sprite.rotation = spriteJson.value("rotation", 0.0f);
                sprite.rotationSpeed = spriteJson.value("rotationSpeed", 0.0f);

                m_sprites.push_back(sprite);
            }
            LOG_INFO("Scene2DTest", "  - 加载精灵数量: {0}", m_sprites.size());
            return true;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Scene2DTest", "解析场景文件失败: {0}", e.what());
    }

    return false;
}

} // namespace Prisma
