#pragma once
#include "../MCPTool.h"

namespace Prisma {
namespace MCP {

class DebugFrameStatsTool : public MCPTool {
public:
    DebugFrameStatsTool();
    std::string_view GetName() const override { return "debug_frame_stats"; }
    std::string_view GetDescription() const override {
        return "Get frame statistics: FPS, draw calls, triangles, VRAM usage.";
    }
    std::string_view GetCategory() const override { return "debug"; }
    glz::json_t GetInputSchema() const override { return {{"type", "object"}, {"properties", glz::json_t::object_t{}}}; }
    glz::json_t Execute(const glz::json_t& args) override;
};

class DebugLogGetTool : public MCPTool {
public:
    DebugLogGetTool();
    std::string_view GetName() const override { return "debug_log_get"; }
    std::string_view GetDescription() const override {
        return "Get engine log entries with level filtering and since-tick pagination.";
    }
    std::string_view GetCategory() const override { return "debug"; }
    glz::json_t GetInputSchema() const override;
    glz::json_t Execute(const glz::json_t& args) override;
};

} // namespace MCP
} // namespace Prisma
