#include "HeadlessRunner.h"
#include "MLGISystem.h"

#include "graphic/pipelines/pathtracing/PathTracingPipeline.h"
#include "Logger.h"

#include <format>

namespace Prisma {

HeadlessRunner::HeadlessRunner(bool enabled, uint32_t totalFrames, const std::string& outputPath,
                               Graphic::PathTracingPipeline* pipeline,
                               MLGISystem* mlgiSystem)
    : m_enabled(enabled)
    , m_totalFrames(totalFrames)
    , m_outputPath(outputPath)
    , m_pipeline(pipeline)
    , m_mlgiSystem(mlgiSystem)
{
}

void HeadlessRunner::LogMLGIMetrics() {
    if (!m_mlgiSystem) {
        LOG_INFO("HeadlessRunner", "MLGI metrics: N/A (no MLGI system)");
        return;
    }

    auto m = m_mlgiSystem->GetMetrics();

    if (m.enabled) {
        LOG_INFO("HeadlessRunner",
                 "MLGI metrics: enabled=true  rayQuery={}  probes={}  raysPerProbe={}  "
                 "temporalFrame={}  nonZeroCoeffs={}/{}",
                 m.rayQueryAvailable ? "yes" : "no",
                 m.probeCount, m.raysPerProbe,
                 m.temporalFrameCount, m.nonZeroCoeffCount, m.totalCoeffCount);
    } else {
        std::string reason = m.fallbackReason.empty() ? "unknown" : m.fallbackReason;
        LOG_INFO("HeadlessRunner",
                 "MLGI metrics: enabled=false  rayQuery={}  fallbackReason=\"{}\"",
                 m.rayQueryAvailable ? "yes" : "no", reason);
    }
}

bool HeadlessRunner::Update() {
    if (!m_enabled || !m_pipeline) return false;

    if (!m_metricsLogged) {
        LogMLGIMetrics();
        m_metricsLogged = true;
    }

    if (m_pipeline->GetFrameCount() >= m_totalFrames) {
        LOG_INFO("MLGIPipeline", "headless模式完成，保存输出...");
        m_pipeline->SaveOutput(m_outputPath);
        return true;
    }

    return false;
}

} // namespace Prisma
