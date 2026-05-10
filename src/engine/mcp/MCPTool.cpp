#include "MCPTool.h"

namespace Prisma {
namespace MCP {

void ToolRegistry::AddTool(std::unique_ptr<MCPTool> tool) {
    if (tool) {
        m_Tools[std::string(tool->GetName())] = std::move(tool);
    }
}

void ToolRegistry::AddTools(std::vector<std::unique_ptr<MCPTool>> tools) {
    for (auto& tool : tools) {
        AddTool(std::move(tool));
    }
}

MCPTool* ToolRegistry::FindTool(const std::string& name) const {
    auto it = m_Tools.find(name);
    return (it != m_Tools.end()) ? it->second.get() : nullptr;
}

std::vector<MCPTool*> ToolRegistry::GetToolList(const std::string& category) const {
    std::vector<MCPTool*> result;
    for (const auto& [name, tool] : m_Tools) {
        if (category.empty() || tool->GetCategory() == category) {
            result.push_back(tool.get());
        }
    }
    std::sort(result.begin(), result.end(), [](MCPTool* a, MCPTool* b) {
        return a->GetName() < b->GetName();
    });
    return result;
}

std::vector<std::string> ToolRegistry::GetCategories() const {
    std::unordered_set<std::string> cats;
    for (const auto& [name, tool] : m_Tools) {
        cats.insert(std::string(tool->GetCategory()));
    }
    std::vector<std::string> result(cats.begin(), cats.end());
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace MCP
} // namespace Prisma
