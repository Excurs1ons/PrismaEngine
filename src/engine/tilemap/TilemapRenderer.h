#pragma once

#include "Tilemap.h"
#include "graphic/OrthographicCamera.h"
#include "graphic/interfaces/ITexture.h"
#include <memory>

namespace Prisma::Tilemap {

class TilemapRenderer {
public:
    TilemapRenderer() = default;

    void SetTilemap(std::shared_ptr<Tilemap> tilemap) { m_tilemap = std::move(tilemap); }
    std::shared_ptr<Tilemap> GetTilemap() const { return m_tilemap; }

    void Render(const Graphic::OrthographicCamera& camera);

    void SetTexture(std::shared_ptr<Graphic::ITexture> texture) { m_texture = std::move(texture); }

private:
    std::shared_ptr<Tilemap> m_tilemap;
    std::shared_ptr<Graphic::ITexture> m_texture;
};

} // namespace Prisma::Tilemap
