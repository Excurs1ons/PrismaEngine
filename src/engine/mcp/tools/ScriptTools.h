#pragma once
#include "../MCPTool.h"

namespace Prisma { class Engine; }

namespace Prisma {
namespace MCP {

class ScriptCompileTool : public MCPTool {
public:
    explicit ScriptCompileTool(Engine* engine);
    std::string_view GetName() const override { return "script_compile"; }
    std::string_view GetDescription() const override {
        return "Compile scripts in the project directory. Returns compilation status.";
    }
    std::string_view GetCategory() const override { return "script"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

class ScriptHotReloadTool : public MCPTool {
public:
    explicit ScriptHotReloadTool(Engine* engine);
    std::string_view GetName() const override { return "script_hot_reload"; }
    std::string_view GetDescription() const override {
        return "Hot reload modified scripts without restarting the engine.";
    }
    std::string_view GetCategory() const override { return "script"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

} // namespace MCP
} // namespace Prisma
