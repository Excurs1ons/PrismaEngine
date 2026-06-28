#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <format>

#include "Logger.h"

namespace Prisma {

// ============================================================================
// ProbeGrid — Application-layer probe grid definition for MLGI
//
// Models a uniform 3D grid of irradiance/distance probes used for
// multi-level global illumination. The grid is defined by its 3D dimensions,
// world-space origin, and probe spacing.
//
// Maximum probe count is clamped to kMaxProbes to prevent accidental
// huge allocations (256^3 = 16M probes would be catastrophic).
// ============================================================================

struct ProbeGrid {
    // ---- Constants ----

    /// Maximum allowed probes (16^3 = 4096, a reasonable MVP upper bound).
    static constexpr uint32_t kMaxProbes = 4096;

    // ---- Grid definition ----

    /// Number of probes along each axis (x, y, z).
    glm::uvec3 dimensions = glm::uvec3(1, 1, 1);

    /// World-space origin (minimum corner) of the grid.
    glm::vec3 origin = glm::vec3(0.0f);

    /// World-space distance between adjacent probes along each axis.
    glm::vec3 spacing = glm::vec3(1.0f);

    /// Total number of probes = dimensions.x * dimensions.y * dimensions.z.
    uint32_t probeCount = 1;

    // ---- Budgeted update state ----

    /// Number of probes to update per frame (stride).
    uint32_t updateStride = 1;

    /// Frame counter for round-robin probe updates (0 .. updateStride-1).
    uint32_t frameIndex = 0;

    // ---- Construction ----

    ProbeGrid() = default;

    /// Construct a grid with the given dimensions, origin, and spacing.
    /// Validates dimensions and clamps to kMaxProbes.
    /// Logs a warning if dimensions are zero or exceed the max.
    ProbeGrid(const glm::uvec3& dim, const glm::vec3& org, const glm::vec3& spc)
        : origin(org)
        , spacing(spc)
    {
        // Reject zero dimensions — a grid with zero probes is useless.
        if (dim.x == 0 || dim.y == 0 || dim.z == 0) {
            LOG_WARN("ProbeGrid", "零维度网格被拒绝 ({}x{}x{}), 回退到 1x1x1",
                     dim.x, dim.y, dim.z);
            dimensions = glm::uvec3(1, 1, 1);
            probeCount = 1;
            updateStride = 1;
            return;
        }

        dimensions = dim;
        probeCount = dim.x * dim.y * dim.z;

        // Clamp to max probes to prevent catastrophic allocations.
        if (probeCount > kMaxProbes) {
            LOG_WARN("ProbeGrid", "探测数 {} 超过上限 {}，已裁剪",
                     probeCount, kMaxProbes);
            // Scale down dimensions proportionally to stay within budget.
            float scale = std::cbrt(static_cast<float>(kMaxProbes) / static_cast<float>(probeCount));
            dimensions = glm::uvec3(
                std::max(1u, static_cast<uint32_t>(dim.x * scale)),
                std::max(1u, static_cast<uint32_t>(dim.y * scale)),
                std::max(1u, static_cast<uint32_t>(dim.z * scale))
            );
            probeCount = dimensions.x * dimensions.y * dimensions.z;
        }

        updateStride = std::max(1u, probeCount / 64); // heuristic: 1/64th per frame
    }

    // ---- Logging ----

    /// Print grid configuration to the log.
    void LogConfig() const {
        LOG_INFO("ProbeGrid",
                 "网格配置: 尺寸={}x{}x{} 原点=({:.1f},{:.1f},{:.1f}) "
                 "间距=({:.1f},{:.1f},{:.1f}) 探测数={} 每帧更新={}",
                 dimensions.x, dimensions.y, dimensions.z,
                 origin.x, origin.y, origin.z,
                 spacing.x, spacing.y, spacing.z,
                 probeCount, updateStride);
    }
};

} // namespace Prisma
