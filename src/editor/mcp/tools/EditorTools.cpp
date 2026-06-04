#include "EditorTools.h"
#include "app/Engine.h"
#include "core/LayerStack.h"

namespace Prisma {
namespace MCP {

EditorSelectionTool::EditorSelectionTool() = default;

glz::json_t EditorSelectionTool::Execute(const glz::json_t& /*args*/) {
    uint64_t entityId = 0;
    std::string entityName;
    bool editorReady = false;

    auto& engine = Engine::Get();
    if (engine.IsRunning()) {
        auto& app = Application::Get();
        for (const auto* layer : app.GetLayerStack()) {
            if (layer && layer->GetName() == "EditorLayer") {
                editorReady = true;
                break;
            }
        }
    }

    glz::json_t result = {
        {"selected_entity_id", static_cast<double>(entityId)},
        {"selected_entity_name", entityName}
    };
    if (!editorReady) {
        result["note"] = "Editor selection state not yet wired — EditorLayer selection API pending";
    }
    return result;
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
