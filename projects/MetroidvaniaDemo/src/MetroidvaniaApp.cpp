#include "MetroidvaniaApp.h"
#include "graphic/Renderer2D.h"
#include "graphic/interfaces/IResourceManager.h"
#include "app/Engine.h"
#include "core/AssetManager.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "transform/Camera.h"
#include "transform/Transform.h"
#include "core/Event.h"
#include "core/Node.h"
#include "core/EntityManager.h"
#include "core/SpriteRendererComponent.h"
#include "physics/PhysicsSystem.h"
#include "tilemap/Tilemap.h"
#include "Logger.h"
#include "Platform.h"
#include <SDL3/SDL_scancode.h>
#include <algorithm>
#include <cmath>
#include <filesystem>

#if PRISMA_ENABLE_SCRIPTING > 0
#include "scripting/ScriptEngine.h"
#endif

namespace Prisma {

namespace {

constexpr const char* kTilemapPath = "assets/maps/test_dungeon.json";

std::filesystem::path ResolveAssetPath(const std::string& relativePath) {
    std::filesystem::path requested(relativePath);
    if (requested.is_absolute() && std::filesystem::exists(requested)) {
        return requested;
    }

    if (auto* assetManager = Engine::Get().GetAssetManager()) {
        if (auto found = assetManager->FindResource(relativePath)) {
            return *found;
        }
    }

    const std::filesystem::path exeDir = Platform::GetExecutablePath();
    const std::filesystem::path candidates[] = {
        requested,
        exeDir / requested,
        std::filesystem::path("projects/MetroidvaniaDemo") / requested
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return requested;
}

} // namespace

MetroidvaniaApp::MetroidvaniaApp()
    : Application({"MetroidvaniaDemo", "", 1024, 896, true, true, Graphic::PresentMode::Mailbox, 0})
{
}

int MetroidvaniaApp::OnInitialize() {
    LOG_INFO("Metroidvania", "Scene-driven demo initialized");

    auto* sceneMgr = Engine::Get().GetSceneManager();
    auto* scene = sceneMgr ? sceneMgr->GetCurrentScene() : nullptr;

    if (!scene) {
        sceneMgr->CreateNewScene();
        scene = sceneMgr->GetCurrentScene();
    }

    if (scene) {
        auto camera = scene->GetMainCamera();
        if (camera) {
            camera->SetViewport(m_Spec.Width, m_Spec.Height);
        }
    }

    // 设置物理系统配置
    auto* physics = Engine::Get().GetPhysicsSystem();
    if (physics) {
        physics->setGravity({0.0, -900.0, 0.0}); // 较强的重力以获得更好的手感
    }

    InitializeTilemap();

    return 0;
}

void MetroidvaniaApp::InitializeTilemap() {
    const auto tilemapPath = ResolveAssetPath(kTilemapPath);
    m_tilemap = std::make_shared<Tilemap::Tilemap>();

    if (!m_tilemap->LoadFromJSON(tilemapPath.string())) {
        LOG_ERROR("Metroidvania", "Failed to load tilemap: {0}", tilemapPath.string());
        m_tilemap.reset();
        return;
    }

    std::string texturePath = m_tilemap->GetTileSet().GetTexturePath();
    if (texturePath.empty()) {
        texturePath = "assets/textures/tiles.png";
    }

    const auto resolvedTexturePath = ResolveAssetPath(texturePath);
    auto* resources = Engine::Get().GetRenderResourceManager();
    if (resources) {
        m_tileTexture = resources->LoadTexture(resolvedTexturePath.string(), false);
    }

    if (!m_tileTexture) {
        LOG_ERROR("Metroidvania", "Failed to load tile texture: {0}", resolvedTexturePath.string());
    }
}

void MetroidvaniaApp::OnUpdate(Timestep ts) {
    if (m_autoQuit) {
        m_elapsedTime += ts;
        if (m_elapsedTime >= m_autoExitTimeout) {
            LOG_INFO("Metroidvania", "Auto-exit after {0:.1f}s", m_elapsedTime);
            Close();
        }
    }
}

void MetroidvaniaApp::SyncCameraFromScripts(const std::shared_ptr<Graphic::Camera>& camera) {
    if (!camera) return;

#if PRISMA_ENABLE_SCRIPTING > 0
    auto& scriptEngine = Engine::Get().GetScriptEngine();
    if (scriptEngine.IsInitialized()) {
        float x = 0.0f;
        float y = 0.0f;
        scriptEngine.GetCameraPos(&x, &y);

        if (auto transform = camera->GetTransform()) {
            transform->SetPosition({x, y, 0.0f});
        }
    }
#else
    (void)camera;
#endif
}

void MetroidvaniaApp::DrawTilemap(const Graphic::Camera& camera) {
    if (!m_tilemap || !m_tileTexture) return;

    const float tileSize = static_cast<float>(m_tilemap->GetTileSize());
    if (tileSize <= 0.0f) return;

    const auto cameraPos = camera.GetPosition();
    const float halfH = camera.GetOrthoSize() * 0.5f;
    const float halfW = halfH * camera.GetAspectRatio();

    int startX = static_cast<int>(std::floor((cameraPos.x - halfW - tileSize) / tileSize));
    int endX = static_cast<int>(std::ceil((cameraPos.x + halfW + tileSize) / tileSize));
    int startY = static_cast<int>(std::floor((cameraPos.y - halfH - tileSize) / tileSize));
    int endY = static_cast<int>(std::ceil((cameraPos.y + halfH + tileSize) / tileSize));

    startX = std::max(0, startX);
    startY = std::max(0, startY);
    endX = std::min(static_cast<int>(m_tilemap->GetWidth()), endX);
    endY = std::min(static_cast<int>(m_tilemap->GetHeight()), endY);

    uint32_t columns = m_tilemap->GetTileSet().GetColumns();
    if (columns == 0) columns = 1;

    for (uint32_t layerIndex = 0; layerIndex < m_tilemap->GetLayerCount(); ++layerIndex) {
        const auto* layer = m_tilemap->GetLayer(layerIndex);
        if (!layer || !layer->visible) continue;

        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                const uint32_t tileId = layer->GetTile(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
                if (tileId == 0) continue;

                const auto* tileDef = m_tilemap->GetTileSet().GetTileDef(tileId);
                const uint32_t texIndex = tileDef ? tileDef->texIndex : 0;
                const uint32_t tx = texIndex % columns;
                const uint32_t ty = texIndex / columns;

                const float u0 = static_cast<float>(tx) / static_cast<float>(columns);
                const float v0 = static_cast<float>(ty) / static_cast<float>(columns);
                const float u1 = u0 + 1.0f / static_cast<float>(columns);
                const float v1 = v0 + 1.0f / static_cast<float>(columns);

                const Vector2 uv[4] = {
                    {u0, v0},
                    {u1, v0},
                    {u1, v1},
                    {u0, v1}
                };

                const Vector2 position(
                    static_cast<float>(x) * tileSize + tileSize * 0.5f,
                    static_cast<float>(y) * tileSize + tileSize * 0.5f);
                Graphic::Renderer2D::DrawQuad(position, {tileSize, tileSize}, m_tileTexture, uv);
            }
        }
    }
}

void MetroidvaniaApp::OnRender() {
    auto* scene = Engine::Get().GetSceneManager()->GetCurrentScene();
    auto camera = scene ? scene->GetMainCamera() : nullptr;
    if (camera) {
        auto cam = std::dynamic_pointer_cast<Graphic::Camera>(camera);
        if (cam) {
            SyncCameraFromScripts(cam);
            cam->SetViewport(m_Spec.Width, m_Spec.Height);
        }

        Graphic::Renderer2D::BeginScene(*camera);
        if (cam) {
            DrawTilemap(*cam);
        }
        Graphic::Renderer2D::DrawNodesSoA();
    }
    Graphic::Renderer2D::EndScene();
}

void MetroidvaniaApp::OnEvent(Event& e) {
    Application::OnEvent(e);

    EventDispatcher d(e);
    d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        if (ev.GetKeyCode() == SDL_SCANCODE_ESCAPE) {
            LOG_INFO("Metroidvania", "ESC pressed — closing");
            Close();
            return true;
        }
        return false;
    });
}

} // namespace Prisma
