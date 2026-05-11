#pragma once
#include "../MCPTool.h"

namespace Prisma { class Engine; }

namespace Prisma {
namespace MCP {

class PropertyGetTool : public MCPTool {
public:
    explicit PropertyGetTool(Engine* engine);
    std::string_view GetName() const override { return "property_get"; }
    std::string_view GetDescription() const override {
        return "Get a property value from an entity component. Supports transform (position, rotation, scale) and mesh_renderer (material).";
    }
    std::string_view GetCategory() const override { return "property"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

class PropertySetTool : public MCPTool {
public:
    explicit PropertySetTool(Engine* engine);
    std::string_view GetName() const override { return "property_set"; }
    std::string_view GetDescription() const override {
        return "Set a property value on an entity component. Supports transform (position, rotation, scale) and mesh_renderer (material).";
    }
    std::string_view GetCategory() const override { return "property"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

} // namespace MCP
} // namespace Prisma