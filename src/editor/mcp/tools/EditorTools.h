#pragma once
#include "../MCPTool.h"

namespace Prisma {
namespace MCP {

class EditorSelectionTool : public MCPTool {
public:
    EditorSelectionTool();
    std::string_view GetName() const override { return "editor_get_selection"; }
    std::string_view GetDescription() const override { return "Get the currently selected entity in the editor."; }
    std::string_view GetCategory() const override { return "editor"; }
    glz::json_t GetInputSchema() const override { return {{"type", "object"}, {"properties", glz::json_t::object_t{}}}; }
    glz::json_t Execute(const glz::json_t& args) override;
};

class EditorConsoleTool : public MCPTool {
public:
    EditorConsoleTool();
    std::string_view GetName() const override { return "editor_console_get"; }
    std::string_view GetDescription() const override { return "Get editor console output."; }
    std::string_view GetCategory() const override { return "editor"; }
    glz::json_t GetInputSchema() const override;
    glz::json_t Execute(const glz::json_t& args) override;
};

} // namespace MCP
} // namespace Prisma
