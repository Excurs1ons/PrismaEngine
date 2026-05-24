#include "DebugTools.h"

namespace Prisma {
namespace MCP {

DebugFrameStatsTool::DebugFrameStatsTool() = default;

glz::json_t DebugFrameStatsTool::Execute(const glz::json_t& /*args*/) {
    // TODO: Hook into actual engine frame stat counters
    return {{"note", "Frame stats not yet implemented - placeholder"},
            {"fps", 0},
            {"draw_calls", 0},
            {"triangles", 0}};
}

DebugLogGetTool::DebugLogGetTool() = default;

glz::json_t DebugLogGetTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"level", {{"type", "string"}, {"description", "Filter: trace, debug, info, warn, error"}}},
            {"since_tick", {{"type", "integer"}, {"description", "Start tick for pagination"}}},
            {"limit", {{"type", "integer"}, {"description", "Max entries (default 50)"}}}
        }}
    };
}

glz::json_t DebugLogGetTool::Execute(const glz::json_t& /*args*/) {
    // TODO: Hook into Logger ring buffer
    return {{"entries", std::vector<glz::json_t>{}}};
}

} // namespace MCP
} // namespace Prisma
