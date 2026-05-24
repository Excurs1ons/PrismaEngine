#include "HeadlessRunner.h"

#include "graphic/pipelines/pathtracing/PathTracingPipeline.h"
#include "Logger.h"

namespace Prisma {

HeadlessRunner::HeadlessRunner(bool enabled, uint32_t totalFrames, const std::string& outputPath,
                               Graphic::PathTracingPipeline* pipeline)
    : m_enabled(enabled)
    , m_totalFrames(totalFrames)
    , m_outputPath(outputPath)
    , m_pipeline(pipeline)
{
}

bool HeadlessRunner::Update() {
    if (!m_enabled || !m_pipeline) return false;

    if (m_pipeline->GetFrameCount() >= m_totalFrames) {
        LOG_INFO("PathTracing3D", "headless模式完成，保存输出...");
        m_pipeline->SaveOutput(m_outputPath);
        return true;
    }

    return false;
}

} // namespace Prisma
