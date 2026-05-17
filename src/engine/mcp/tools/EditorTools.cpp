#include "EditorTools.h"

namespace Prisma {
namespace MCP {

EditorSelectionTool::EditorSelectionTool() = default;

glz::json_t EditorSelectionTool::Execute(const glz::json_t& /*args*/) {
    // TODO: Integrate with Editor selection state
    return {{"selected_entity_id", 0}, {"selected_entity_name", ""}};
}

EditorConsoleTool::EditorConsoleTool() = default;

glz::json_t EditorConsoleTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"limit", {{"type", "integer"}, {"description", "Max entries (default 50)"}}}
        }}
    };
}

glz::json_t EditorConsoleTool::Execute(const glz::json_t& /*args*/) {
    return {{"entries", std::vector<glz::json_t>{}}};
}

} // namespace MCP
} // namespace Prisma
