#pragma once
#include "../MCPTool.h"

namespace Prisma {

class Engine;

namespace MCP {

class DebugFrameStatsTool : public MCPTool {
public:
    DebugFrameStatsTool();
    std::string_view GetName() const override { return "debug_frame_stats"; }
    std::string_view GetDescription() const override {
        return "Get frame statistics: FPS, draw calls, triangles, VRAM usage.";
    }
    std::string_view GetCategory() const override { return "debug"; }
    nlohmann::json GetInputSchema() const override { return {{"type", "object"}, {"properties", {}}}; }
    nlohmann::json Execute(const nlohmann::json& args) override;
};

class DebugLogGetTool : public MCPTool {
public:
    DebugLogGetTool();
    std::string_view GetName() const override { return "debug_log_get"; }
    std::string_view GetDescription() const override {
        return "Get engine log entries with level filtering and since-tick pagination.";
    }
    std::string_view GetCategory() const override { return "debug"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
};

class DebugProfilerGetTool : public MCPTool {
public:
    DebugProfilerGetTool(Engine* engine);
    std::string_view GetName() const override { return "debug_profiler_get"; }
    std::string_view GetDescription() const override {
        return "Get profiler data including FPS, frame time, and render time breakdown.";
    }
    std::string_view GetCategory() const override { return "debug"; }
    nlohmann::json GetInputSchema() const override { return {{"type", "object"}, {"properties", {}}}; }
    nlohmann::json Execute(const nlohmann::json& args) override;

private:
    Engine* m_Engine;
};

class DebugBreakpointListTool : public MCPTool {
public:
    DebugBreakpointListTool();
    std::string_view GetName() const override { return "debug_breakpoint_list"; }
    std::string_view GetDescription() const override {
        return "List all registered breakpoints.";
    }
    std::string_view GetCategory() const override { return "debug"; }
    nlohmann::json GetInputSchema() const override { return {{"type", "object"}, {"properties", {}}}; }
    nlohmann::json Execute(const nlohmann::json& args) override;
};

class DebugBreakpointToggleTool : public MCPTool {
public:
    DebugBreakpointToggleTool();
    std::string_view GetName() const override { return "debug_breakpoint_toggle"; }
    std::string_view GetDescription() const override {
        return "Enable or disable a breakpoint by ID.";
    }
    std::string_view GetCategory() const override { return "debug"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
};

} // namespace MCP
} // namespace Prisma
