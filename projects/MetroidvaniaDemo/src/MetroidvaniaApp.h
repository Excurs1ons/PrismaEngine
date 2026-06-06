#pragma once

#include "app/Application.h"
#include "core/Node.h"
#include <memory>

namespace Prisma::Graphic {
class Camera;
class ITexture;
}

namespace Prisma::Tilemap {
class Tilemap;
}

namespace Prisma {

class MetroidvaniaApp : public Application {
public:
    MetroidvaniaApp();
    ~MetroidvaniaApp() override = default;

    void SetAutoQuit(bool quit) { m_autoQuit = quit; }

    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    void InitializeTilemap();
    void SyncCameraFromScripts(const std::shared_ptr<Graphic::Camera>& camera);
    void DrawTilemap(const Graphic::Camera& camera);

    std::shared_ptr<Tilemap::Tilemap> m_tilemap;
    std::shared_ptr<Graphic::ITexture> m_tileTexture;

    bool m_autoQuit = false;
    float m_elapsedTime = 0;
    float m_autoExitTimeout = 30.0f;
};

} // namespace Prisma
