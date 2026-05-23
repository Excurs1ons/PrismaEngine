#include "Deferred3DApp.h"
#include "DeferredPipelineAdapter.h"
#include "StatsOverlay.h"

#include "graphic/RenderSystem.h"
#include "graphic/Renderer2D.h"
#include "app/Engine.h"
#include "input/InputManager.h"
#include "platform/Platform.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "Logger.h"

#include <algorithm>

namespace Prisma {
using namespace Graphic;

Deferred3DApp::Deferred3DApp()
    : Application()
{
}

Deferred3DApp::~Deferred3DApp() = default;

int Deferred3DApp::OnInitialize()
{
    LOG_INFO("Deferred3D", "deferred rendering debug project");

    auto* sceneManager = Engine::Get().GetSceneManager();
    if (sceneManager) {
        m_scene = sceneManager->GetCurrentScene();
    }

    auto* renderSystem = Engine::Get().GetRenderSystem();
    if (!renderSystem) {
        LOG_ERROR("Deferred3D", "no render system");
        return -1;
    }

    auto adapter = std::make_shared<DeferredPipelineAdapter>();
    if (adapter->Initialize(renderSystem->GetDevice()) != 0) {
        LOG_ERROR("Deferred3D", "deferred adapter init failed");
        return -1;
    }

    m_deferredAdapter = adapter;
    renderSystem->SetMainPipeline(adapter);

    m_scenePath = m_Spec.EntryScene;
    m_sceneList = m_Spec.Scenes;
    if (!m_sceneList.empty()) {
        auto it = std::find(m_sceneList.begin(), m_sceneList.end(), m_scenePath);
        m_currentSceneIndex = (it != m_sceneList.end())
            ? static_cast<int>(std::distance(m_sceneList.begin(), it))
            : 0;
    }

    if (m_scene) {
        auto overlayNode = m_scene->CreateNode("StatsOverlay");
        m_statsOverlay = std::make_unique<StatsOverlay>(m_Spec);
        m_statsOverlay->SetOwnerNode(overlayNode, m_scene);
    }

    return 0;
}

void Deferred3DApp::LoadScene(const std::string& path)
{
    auto* sceneManager = Engine::Get().GetSceneManager();
    if (!sceneManager) return;

    if (!sceneManager->LoadFromFile(path)) {
        LOG_ERROR("Deferred3D", "scene load failed: {}", path);
        return;
    }

    m_scene = sceneManager->GetCurrentScene();
    m_scenePath = path;
    LOG_INFO("Deferred3D", "scene switched: {}", path);
}

void Deferred3DApp::OnRender()
{
    if (m_statsOverlay) {
        m_statsOverlay->Update(Timestep{});
    }
}

void Deferred3DApp::OnUpdate(Timestep ts)
{
    if (m_scene && m_scene->IsDirty()) {
        m_scene->SetDirty(false);
    }
}

void Deferred3DApp::OnEvent(Event& e)
{
    Application::OnEvent(e);

    EventDispatcher d(e);
    d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        auto key = static_cast<Input::KeyCode>(ev.GetKeyCode());
        bool repeat = ev.IsRepeat();

        if (key == Input::KeyCode::Escape) {
            Close();
            return true;
        }
        if (key == Input::KeyCode::G && !repeat) {
            m_showGBuffer = !m_showGBuffer;
            LOG_INFO("Deferred3D", "GBuffer overlay {}", m_showGBuffer ? "on" : "off");
            return true;
        }
        if (key == Input::KeyCode::LeftBracket && !repeat) {
            m_gBufferTarget = (m_gBufferTarget - 1 + 5) % 5;
            LOG_INFO("Deferred3D", "GBuffer target: {}", m_gBufferTarget);
            return true;
        }
        if (key == Input::KeyCode::RightBracket && !repeat) {
            m_gBufferTarget = (m_gBufferTarget + 1) % 5;
            LOG_INFO("Deferred3D", "GBuffer target: {}", m_gBufferTarget);
            return true;
        }
        if (key == Input::KeyCode::F5 && !repeat) {
            LOG_INFO("Deferred3D", "scene reload: {}", m_scenePath);
            LoadScene(m_scenePath);
            return true;
        }
        if (key == Input::KeyCode::F6 && !repeat && !m_sceneList.empty()) {
            m_currentSceneIndex = (m_currentSceneIndex + 1) % static_cast<int>(m_sceneList.size());
            LOG_INFO("Deferred3D", "scene next: [{} / {}] {}",
                     m_currentSceneIndex + 1, m_sceneList.size(), m_sceneList[m_currentSceneIndex]);
            LoadScene(m_sceneList[m_currentSceneIndex]);
            return true;
        }
        if (key == Input::KeyCode::F7 && !repeat && !m_sceneList.empty()) {
            m_currentSceneIndex = (m_currentSceneIndex - 1 + static_cast<int>(m_sceneList.size())) % static_cast<int>(m_sceneList.size());
            LOG_INFO("Deferred3D", "scene prev: [{} / {}] {}",
                     m_currentSceneIndex + 1, m_sceneList.size(), m_sceneList[m_currentSceneIndex]);
            LoadScene(m_sceneList[m_currentSceneIndex]);
            return true;
        }
        return false;
    });
}

} // namespace Prisma
