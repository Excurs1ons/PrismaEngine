#pragma once
#include "../MCPTool.h"

namespace Prisma { class Engine; }

namespace Prisma {
namespace MCP {

class ConsoleExecuteTool : public MCPTool {
public:
    explicit ConsoleExecuteTool(Engine* engine);
    std::string_view GetName() const override { return "console_execute"; }
    std::string_view GetDescription() const override {
        return "Execute a console command. Returns command output or error message.";
    }
    std::string_view GetCategory() const override { return "console"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

class LogFilterTool : public MCPTool {
public:
    explicit LogFilterTool(Engine* engine);
    std::string_view GetName() const override { return "log_filter"; }
    std::string_view GetDescription() const override {
        return "Filter engine logs by level and category. Returns matching log entries.";
    }
    std::string_view GetCategory() const override { return "console"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

} // namespace MCP
} // namespace Prisma
