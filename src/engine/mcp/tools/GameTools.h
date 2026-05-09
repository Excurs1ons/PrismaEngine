#pragma once
#include "../MCPTool.h"

namespace Prisma {
namespace MCP {

class GameGetStateTool : public MCPTool {
public:
    GameGetStateTool();
    std::string_view GetName() const override { return "game_get_state"; }
    std::string_view GetDescription() const override { return "Get current game runtime state."; }
    std::string_view GetCategory() const override { return "game"; }
    nlohmann::json GetInputSchema() const override { return {{"type", "object"}, {"properties", {}}}; }
    nlohmann::json Execute(const nlohmann::json& args) override;
};

class GameSimulateTool : public MCPTool {
public:
    GameSimulateTool();
    std::string_view GetName() const override { return "game_simulate"; }
    std::string_view GetDescription() const override { return "Advance the game simulation by N frames."; }
    std::string_view GetCategory() const override { return "game"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
};

} // namespace MCP
} // namespace Prisma
