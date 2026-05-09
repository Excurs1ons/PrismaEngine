#include "GameTools.h"

namespace Prisma {
namespace MCP {

GameGetStateTool::GameGetStateTool() = default;

nlohmann::json GameGetStateTool::Execute(const nlohmann::json& /*args*/) {
    return {{"state", "unknown"}, {"running", false}};
}

GameSimulateTool::GameSimulateTool() = default;

nlohmann::json GameSimulateTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"frames", {{"type", "integer"}, {"description", "Number of frames to simulate"}}}
        }},
        {"required", {"frames"}}
    };
}

nlohmann::json GameSimulateTool::Execute(const nlohmann::json& /*args*/) {
    return {{"error", "Not implemented"}};
}

} // namespace MCP
} // namespace Prisma
