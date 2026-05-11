#include "DebugTools.h"
#include "app/Engine.h"

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

DebugProfilerGetTool::DebugProfilerGetTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json DebugProfilerGetTool::Execute(const nlohmann::json& /*args*/) {
    if (!m_Engine) {
        return {{"error", "Engine not available"}};
    }
    const auto& stats = m_Engine->GetFrameStats();
    return {
        {"fps", stats.FPS},
        {"frame_time_ms", stats.TotalTime},
        {"begin_frame_ms", stats.BeginFrameTime},
        {"render_ms", stats.RenderTime},
        {"end_frame_ms", stats.EndFrameTime},
        {"present_ms", stats.PresentTime}
    };
}

DebugBreakpointListTool::DebugBreakpointListTool() = default;

nlohmann::json DebugBreakpointListTool::Execute(const nlohmann::json& /*args*/) {
    return {
        {"note", "Breakpoint system not yet implemented"},
        {"breakpoints", nlohmann::json::array()}
    };
}

DebugBreakpointToggleTool::DebugBreakpointToggleTool() = default;

nlohmann::json DebugBreakpointToggleTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"id", {{"type", "string"}, {"description", "Breakpoint ID to toggle"}}},
            {"enabled", {{"type", "boolean"}, {"description", "Enable (true) or disable (false)"}}}
        }},
        {"required", {"id"}}
    };
}

nlohmann::json DebugBreakpointToggleTool::Execute(const nlohmann::json& /*args*/) {
    return {{"note", "Breakpoint system not yet implemented"}};
}

} // namespace MCP
} // namespace Prisma
