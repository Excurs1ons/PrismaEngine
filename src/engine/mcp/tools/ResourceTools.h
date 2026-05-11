#pragma once
#include "../MCPTool.h"

namespace Prisma { class Engine; }

namespace Prisma {
namespace MCP {

class ResourceListTool : public MCPTool {
public:
    explicit ResourceListTool(Engine* engine);
    std::string_view GetName() const override { return "resource_list"; }
    std::string_view GetDescription() const override {
        return "List all loaded resources with optional type filter (texture, mesh, shader, audio) and cache status.";
    }
    std::string_view GetCategory() const override { return "resource"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

class ResourceLoadTool : public MCPTool {
public:
    explicit ResourceLoadTool(Engine* engine);
    std::string_view GetName() const override { return "resource_load"; }
    std::string_view GetDescription() const override {
        return "Load a resource by path with optional type hint. Returns the loaded resource info.";
    }
    std::string_view GetCategory() const override { return "resource"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

class ResourceUnloadTool : public MCPTool {
public:
    explicit ResourceUnloadTool(Engine* engine);
    std::string_view GetName() const override { return "resource_unload"; }
    std::string_view GetDescription() const override {
        return "Unload a resource by path from the cache. Resources are automatically refcounted.";
    }
    std::string_view GetCategory() const override { return "resource"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

} // namespace MCP
} // namespace Prisma