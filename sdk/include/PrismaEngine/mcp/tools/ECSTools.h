#pragma once
#include "../MCPTool.h"

namespace Prisma { class Engine; }

namespace Prisma {
namespace MCP {

class ECSComponentListTool : public MCPTool {
public:
    explicit ECSComponentListTool(Engine* engine);
    std::string_view GetName() const override { return "ecs_component_list"; }
    std::string_view GetDescription() const override { return "List all component types registered on an entity."; }
    std::string_view GetCategory() const override { return "ecs"; }
    glz::json_t GetInputSchema() const override;
    glz::json_t Execute(const glz::json_t& args) override;
private:
    Engine* m_Engine;
};

class ECSComponentGetTool : public MCPTool {
public:
    explicit ECSComponentGetTool(Engine* engine);
    std::string_view GetName() const override { return "ecs_component_get"; }
    std::string_view GetDescription() const override { return "Get component data for a specific component type on an entity."; }
    std::string_view GetCategory() const override { return "ecs"; }
    glz::json_t GetInputSchema() const override;
    glz::json_t Execute(const glz::json_t& args) override;
private:
    Engine* m_Engine;
};

class ECSComponentSetTool : public MCPTool {
public:
    explicit ECSComponentSetTool(Engine* engine);
    std::string_view GetName() const override { return "ecs_component_set"; }
    std::string_view GetDescription() const override { return "Set component data for a specific component type on an entity."; }
    std::string_view GetCategory() const override { return "ecs"; }
    glz::json_t GetInputSchema() const override;
    glz::json_t Execute(const glz::json_t& args) override;
private:
    Engine* m_Engine;
};

} // namespace MCP
} // namespace Prisma
