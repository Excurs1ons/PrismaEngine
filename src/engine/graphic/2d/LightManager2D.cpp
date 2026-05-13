#include "LightManager2D.h"
#include <algorithm>

namespace Prisma::Graphic {

LightManager2D& LightManager2D::Get() {
    static LightManager2D instance;
    return instance;
}

uint32_t LightManager2D::CreateLight(Light2D::Type type) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto light = std::make_shared<Light2D>(type);
    
    // 查找空位
    for (size_t i = 0; i < m_lights.size(); ++i) {
        if (!m_lights[i]) {
            m_lights[i] = light;
            return static_cast<uint32_t>(i + 1);
        }
    }
    
    m_lights.push_back(light);
    return static_cast<uint32_t>(m_lights.size());
}

void LightManager2D::DestroyLight(uint32_t handle) {
    if (handle == 0) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    uint32_t index = handle - 1;
    if (index < m_lights.size()) {
        m_lights[index].reset();
    }
}

Light2D* LightManager2D::GetLight(uint32_t handle) {
    if (handle == 0) return nullptr;
    
    uint32_t index = handle - 1;
    if (index < m_lights.size()) {
        return m_lights[index].get();
    }
    return nullptr;
}

} // namespace Prisma::Graphic
