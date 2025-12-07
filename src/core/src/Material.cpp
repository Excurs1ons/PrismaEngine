#include "Material.h"
#include "Logger.h"

using namespace Engine;

void Material::Apply(RenderCommandContext* context)
{
    // Minimal placeholder implementation: log and no-op.
    // Real implementation should bind shaders, descriptor heaps, textures, samplers and constant buffers.
    LOG_DEBUG("Material", "Apply called. Context ptr={0}", reinterpret_cast<uintptr_t>(context));
    if (!context) {
        LOG_WARNING("Material", "Apply: context is null");
        return;
    }
    // TODO: bind shader resources / set pipeline state
}

Material::~Material() {
}

bool Material::Load(const std::filesystem::path& path)
{
    m_path = path;
    m_name = path.filename().string();
    // placeholder: mark loaded
    m_isLoaded = true;
    return true;
}

void Material::Unload()
{
    m_isLoaded = false;
}

bool Material::IsLoaded() const
{
    return m_isLoaded;
}
