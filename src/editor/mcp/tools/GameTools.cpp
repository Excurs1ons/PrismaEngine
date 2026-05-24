#include "GameTools.h"

namespace Prisma {
namespace MCP {

GameGetStateTool::GameGetStateTool() = default;

glz::json_t GameGetStateTool::Execute(const glz::json_t& /*args*/) {
    return {{"state", "unknown"}, {"running", false}};
}

GameSimulateTool::GameSimulateTool() = default;

glz::json_t GameSimulateTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"frames", {{"type", "integer"}, {"description", "Number of frames to simulate"}}}
        }},
        {"required", {"frames"}}
    };
}

glz::json_t GameSimulateTool::Execute(const glz::json_t& /*args*/) {
    return {{"error", "Not implemented"}};
}

} // namespace MCP
} // namespace Prisma
