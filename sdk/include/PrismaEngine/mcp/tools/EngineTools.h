#pragma once
#include "../MCPTool.h"

namespace Prisma { class Engine; }

namespace Prisma {
namespace MCP {

class EngineStatusTool : public MCPTool {
public:
    explicit EngineStatusTool(Engine* engine);
    std::string_view GetName() const override { return "engine_get_status"; }
    std::string_view GetDescription() const override {
        return "Get current engine status: running, FPS, scene info, build configuration.";
    }
    std::string_view GetCategory() const override { return "engine"; }
    glz::json_t GetInputSchema() const override;
    glz::json_t Execute(const glz::json_t& args) override;
private:
    Engine* m_Engine;
};

class EngineStateHashTool : public MCPTool {
public:
    explicit EngineStateHashTool(Engine* engine);
    std::string_view GetName() const override { return "mcp/get_state_hash"; }
    std::string_view GetDescription() const override {
        return "Get the current root state hash for delta-based incremental queries.";
    }
    std::string_view GetCategory() const override { return "engine"; }
    glz::json_t GetInputSchema() const override;
    glz::json_t Execute(const glz::json_t& args) override;
private:
    Engine* m_Engine;
};

class EngineBuildInfoTool : public MCPTool {
public:
    explicit EngineBuildInfoTool(Engine* engine);
    std::string_view GetName() const override { return "engine_get_build_info"; }
    std::string_view GetDescription() const override {
        return "Get build configuration info: compiler, platform, build type, feature flags.";
    }
    std::string_view GetCategory() const override { return "engine"; }
    glz::json_t GetInputSchema() const override;
    glz::json_t Execute(const glz::json_t& args) override;
private:
    Engine* m_Engine;
};

} // namespace MCP
} // namespace Prisma
