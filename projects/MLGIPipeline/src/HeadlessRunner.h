#pragma once

#include <string>
#include <cstdint>

namespace Prisma {

class MLGISystem;
namespace Graphic { class PathTracingPipeline; }

class HeadlessRunner {
public:
    HeadlessRunner(bool enabled, uint32_t totalFrames, const std::string& outputPath,
                   Graphic::PathTracingPipeline* pipeline,
                   MLGISystem* mlgiSystem = nullptr);

    bool Update();

private:
    void LogMLGIMetrics();

    bool m_enabled = false;
    uint32_t m_totalFrames = 100;
    std::string m_outputPath = "output.png";
    Graphic::PathTracingPipeline* m_pipeline = nullptr;
    MLGISystem* m_mlgiSystem = nullptr;
    bool m_metricsLogged = false;
};

} // namespace Prisma
