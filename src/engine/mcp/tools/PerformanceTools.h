#pragma once
#include "../MCPTool.h"

namespace Prisma { class Engine; }

namespace Prisma {
namespace MCP {

class ProfilerCaptureTool : public MCPTool {
public:
    explicit ProfilerCaptureTool(Engine* engine);
    std::string_view GetName() const override { return "profiler_capture"; }
    std::string_view GetDescription() const override {
        return "Capture a performance profile snapshot. Returns frame count and timing data for the specified duration.";
    }
    std::string_view GetCategory() const override { return "performance"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

class MemoryStatsTool : public MCPTool {
public:
    explicit MemoryStatsTool(Engine* engine);
    std::string_view GetName() const override { return "memory_stats"; }
    std::string_view GetDescription() const override {
        return "Get engine memory usage statistics including allocated bytes, freed bytes, active allocations, and VRAM usage.";
    }
    std::string_view GetCategory() const override { return "performance"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

} // namespace MCP
} // namespace Prisma