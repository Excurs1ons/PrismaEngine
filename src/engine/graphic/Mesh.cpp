#include "Mesh.h"
#include "Logger.h"

namespace Prisma::Graphic {

bool Mesh::Load(const std::filesystem::path& path)
{
    // TODO: Implement loading from file
    return true;
}

void Mesh::Unload()
{
    m_SubMeshes.clear();
    m_BoundingBox = BoundingBox();
}

} // namespace Prisma::Graphic
