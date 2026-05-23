#include "ClusteredApp.h"
#include "StatsOverlay.h"
#include "graphic/pipelines/clustered/ClusteredForwardPipeline.h"
#include "graphic/RenderSystem.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "Logger.h"
#include "input/InputManager.h"

#include <iostream>
#include <algorithm>

namespace Prisma {

ClusteredApp::ClusteredApp() = default;
ClusteredApp::~ClusteredApp() = default;

int ClusteredApp::OnInitialize() {
    LOG_INFO("ClusteredForward3D", "Clustered Forward 3D Project Initializing...");

    // 获取管线
    m_pipeline = Engine::Get().GetRenderSystem()->GetMainPipelineAs<Graphic::ClusteredForwardPipeline>();
    if (!m_pipeline) {
        LOG_ERROR("ClusteredForward3D", "获取分块前向渲染管线失败");
        return -1;
    }

    auto* sceneManager = Engine::Get().GetSceneManager();
    if (sceneManager) {
        m_scene = sceneManager->GetCurrentScene();
    }

    // 初始化场景列表
    m_scenePath = m_Spec.EntryScene;
    m_sceneList = m_Spec.Scenes;
    if (!m_sceneList.empty()) {
        auto it = std::find(m_sceneList.begin(), m_sceneList.end(), m_scenePath);
        m_currentSceneIndex = (it != m_sceneList.end())
            ? static_cast<int>(std::distance(m_sceneList.begin(), it))
            : 0;
    }

    // 创建 StatsOverlay
    if (m_scene) {
        auto overlayNode = m_scene->CreateNode("StatsOverlay");
        m_statsOverlay = std::make_unique<StatsOverlay>(m_Spec);
        m_statsOverlay->SetOwnerNode(overlayNode, m_scene);
    }

    return 0;
}

void ClusteredApp::LoadScene(const std::string& path) {
    auto* sceneManager = Engine::Get().GetSceneManager();
    if (!sceneManager) return;

    if (!sceneManager->LoadFromFile(path)) {
        LOG_ERROR("ClusteredForward3D", "场景加载失败: {}", path);
        return;
    }

    m_scene = sceneManager->GetCurrentScene();
    m_scenePath = path;

    LOG_INFO("ClusteredForward3D", "场景已切换: {}", path);
}

void ClusteredApp::OnUpdate(Timestep ts) {
    // 逻辑更新
}

void ClusteredApp::OnRender() {
    if (m_statsOverlay) {
        m_statsOverlay->Update(Timestep{});
    }
}

void ClusteredApp::OnEvent(Event& e) {
    Application::OnEvent(e);

    EventDispatcher d(e);
    d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        auto key = static_cast<Input::KeyCode>(ev.GetKeyCode());
        bool repeat = ev.IsRepeat();

        if (key == Input::KeyCode::Escape) {
            Close();
            return true;
        }
        if (key == Input::KeyCode::F5 && !repeat) {
            LoadScene(m_scenePath);
            return true;
        }
        if (key == Input::KeyCode::F6 && !repeat && !m_sceneList.empty()) {
            m_currentSceneIndex = (m_currentSceneIndex + 1) % static_cast<int>(m_sceneList.size());
            LoadScene(m_sceneList[m_currentSceneIndex]);
            return true;
        }
        if (key == Input::KeyCode::F7 && !repeat && !m_sceneList.empty()) {
            m_currentSceneIndex = (m_currentSceneIndex - 1 + static_cast<int>(m_sceneList.size())) % static_cast<int>(m_sceneList.size());
            LoadScene(m_sceneList[m_currentSceneIndex]);
            return true;
        }
        return false;
    });
}

} // namespace Prisma
