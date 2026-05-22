#pragma once

#include <string>
#include <cstdint>

namespace Prisma {
namespace Graphic { class PathTracingPipeline; }

class HeadlessRunner {
public:
    HeadlessRunner(bool enabled, uint32_t totalFrames, const std::string& outputPath,
                   Graphic::PathTracingPipeline* pipeline);

    // Returns true when headless rendering is complete
    bool Update();

private:
    bool m_enabled = false;
    uint32_t m_totalFrames = 100;
    std::string m_outputPath = "output.png";
    Graphic::PathTracingPipeline* m_pipeline = nullptr;
};

} // namespace Prisma
