#pragma once
#include "../MCPTool.h"

namespace Prisma { class Engine; }

namespace Prisma {
namespace MCP {

class SceneHierarchyTool : public MCPTool {
public:
    explicit SceneHierarchyTool(Engine* engine);
    std::string_view GetName() const override { return "scene_get_hierarchy"; }
    std::string_view GetDescription() const override {
        return "Get the scene entity hierarchy tree with entity IDs, names, and parent relationships.";
    }
    std::string_view GetCategory() const override { return "scene"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
    bool SupportsFieldFilter() const override { return true; }
private:
    Engine* m_Engine;
};

class SceneEntityTool : public MCPTool {
public:
    explicit SceneEntityTool(Engine* engine);
    std::string_view GetName() const override { return "scene_get_entity"; }
    std::string_view GetDescription() const override {
        return "Get detailed information about a specific entity by ID. Supports field-level filtering.";
    }
    std::string_view GetCategory() const override { return "scene"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
    bool SupportsFieldFilter() const override { return true; }
private:
    Engine* m_Engine;
};

class SceneCreateEntityTool : public MCPTool {
public:
    explicit SceneCreateEntityTool(Engine* engine);
    std::string_view GetName() const override { return "scene_create_entity"; }
    std::string_view GetDescription() const override { return "Create a new entity in the current scene."; }
    std::string_view GetCategory() const override { return "scene"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

class SceneDeleteEntityTool : public MCPTool {
public:
    explicit SceneDeleteEntityTool(Engine* engine);
    std::string_view GetName() const override { return "scene_delete_entity"; }
    std::string_view GetDescription() const override { return "Delete an entity from the current scene."; }
    std::string_view GetCategory() const override { return "scene"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

} // namespace MCP
} // namespace Prisma
