#include "DebugTools.h"
#include "app/Engine.h"
#include "logger/LogEntry.h"

namespace Prisma {
namespace MCP {

DebugFrameStatsTool::DebugFrameStatsTool() = default;

glz::json_t DebugFrameStatsTool::Execute(const glz::json_t& /*args*/) {
    auto& engine = Engine::Get();
    const auto& frameStats = engine.GetFrameStats();

    double fps          = frameStats.FPS;
    double frameTimeMs  = frameStats.TotalTime;
    uint32_t drawCalls  = 0;
    uint32_t triangles  = 0;
    uint64_t vramUsage  = 0;

    if (auto* renderSystem = engine.GetRenderSystem()) {
        if (auto* device = renderSystem->GetDevice()) {
            auto renderStats = device->GetRenderStats();
            drawCalls = renderStats.drawCalls;
            triangles = renderStats.triangles;
            vramUsage = renderStats.gpuMemoryUsage;
        }
    }

    float cpuFrameTimeMs = 0.0f;
    if (auto* profiler = engine.GetProfilerSystem()) {
        cpuFrameTimeMs = profiler->GetCurrentCpuFrameTime();
    }

    return {
        {"fps", fps},
        {"frame_time_ms", frameTimeMs},
        {"cpu_frame_time_ms", cpuFrameTimeMs},
        {"draw_calls", static_cast<double>(drawCalls)},
        {"triangles", static_cast<double>(triangles)},
        {"vram_usage_bytes", static_cast<double>(vramUsage)}
    };
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

static LogLevel LevelStringToEnum(std::string_view level) {
    if (level == "trace") return LogLevel::Trace;
    if (level == "debug") return LogLevel::Debug;
    if (level == "info")  return LogLevel::Info;
    if (level == "warn")  return LogLevel::Warning;
    if (level == "error") return LogLevel::Error;
    if (level == "fatal") return LogLevel::Fatal;
    return LogLevel::Trace;
}

glz::json_t DebugLogGetTool::Execute(const glz::json_t& args) {
    int64_t limit = 50;
    if (args.get_object().contains("limit")) {
        limit = args["limit"].get_int64();
    }

    std::string levelFilter;
    if (args.get_object().contains("level")) {
        levelFilter = args["level"].get_string();
    }

    auto& logger = Engine::Get().GetLogger();
    auto logs = logger.GetRecentLogs(static_cast<size_t>(limit));

    glz::json_t entries = std::vector<glz::json_t>{};
    for (const auto& entry : logs) {
        if (!levelFilter.empty()) {
            auto filterLevel = LevelStringToEnum(levelFilter);
            if (entry.level != filterLevel) continue;
        }

        std::string levelStr;
        switch (entry.level) {
            case LogLevel::Trace:   levelStr = "trace"; break;
            case LogLevel::Debug:   levelStr = "debug"; break;
            case LogLevel::Info:    levelStr = "info";  break;
            case LogLevel::Warning: levelStr = "warn";  break;
            case LogLevel::Error:   levelStr = "error"; break;
            case LogLevel::Fatal:   levelStr = "fatal"; break;
        }

        entries.get_array().push_back(glz::json_t{
            {"level", levelStr},
            {"message", entry.message},
            {"category", entry.category},
            {"level_value", static_cast<double>(static_cast<int>(entry.level))}
        });
    }

    return {{"entries", std::move(entries)}, {"count", static_cast<double>(entries.get_array().size())}};
}

} // namespace MCP
} // namespace Prisma
