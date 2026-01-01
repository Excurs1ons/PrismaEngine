#include "LogicalPass.h"

namespace PrismaEngine::Graphic {

LogicalPass::LogicalPass(const char* name)
    : m_name(name)
    , m_priority(0)
    , m_enabled(true)
    , m_renderTarget(nullptr)
    , m_depthStencil(nullptr)
    , m_width(1920)
    , m_height(1080)
    , m_deltaTime(0.0f)
    , m_totalTime(0.0f) {
    m_clearColor[0] = 0.0f;
    m_clearColor[1] = 0.0f;
    m_clearColor[2] = 0.0f;
    m_clearColor[3] = 1.0f;
}

void LogicalPass::SetViewport(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
}



} // namespace PrismaEngine::Graphic
