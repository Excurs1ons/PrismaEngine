#include "EditorTools.h"

namespace Prisma {
namespace MCP {

EditorSelectionTool::EditorSelectionTool() = default;

nlohmann::json EditorSelectionTool::Execute(const nlohmann::json& /*args*/) {
    // TODO: Integrate with Editor selection state
    return {{"selected_entity_id", 0}, {"selected_entity_name", ""}};
}

EditorConsoleTool::EditorConsoleTool() = default;

nlohmann::json EditorConsoleTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"limit", {{"type", "integer"}, {"description", "Max entries (default 50)"}}}
        }}
    };
}

nlohmann::json EditorConsoleTool::Execute(const nlohmann::json& /*args*/) {
    return {{"entries", nlohmann::json::array()}};
}

} // namespace MCP
} // namespace Prisma
