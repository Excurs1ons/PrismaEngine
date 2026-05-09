#include "DebugTools.h"

namespace Prisma {
namespace MCP {

DebugFrameStatsTool::DebugFrameStatsTool() = default;

nlohmann::json DebugFrameStatsTool::Execute(const nlohmann::json& /*args*/) {
    // TODO: Hook into actual engine frame stat counters
    return {{"note", "Frame stats not yet implemented - placeholder"},
            {"fps", 0},
            {"draw_calls", 0},
            {"triangles", 0}};
}

DebugLogGetTool::DebugLogGetTool() = default;

nlohmann::json DebugLogGetTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"level", {{"type", "string"}, {"description", "Filter: trace, debug, info, warn, error"}}},
            {"since_tick", {{"type", "integer"}, {"description", "Start tick for pagination"}}},
            {"limit", {{"type", "integer"}, {"description", "Max entries (default 50)"}}}
        }}
    };
}

nlohmann::json DebugLogGetTool::Execute(const nlohmann::json& /*args*/) {
    // TODO: Hook into Logger ring buffer
    return {{"entries", nlohmann::json::array()}};
}

} // namespace MCP
} // namespace Prisma
