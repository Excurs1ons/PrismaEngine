#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "Export.h"

namespace Prisma {
namespace MCP {

class ENGINE_API MCPTool {
public:
    virtual ~MCPTool() = default;

    virtual std::string_view GetName() const = 0;
    virtual std::string_view GetDescription() const = 0;
    virtual std::string_view GetCategory() const = 0;
    virtual nlohmann::json GetInputSchema() const = 0;
    virtual nlohmann::json Execute(const nlohmann::json& args) = 0;

    virtual bool SupportsFieldFilter() const { return false; }
};

class ENGINE_API ToolRegistry {
public:
    void AddTool(std::unique_ptr<MCPTool> tool);
    void AddTools(std::vector<std::unique_ptr<MCPTool>> tools);

    MCPTool* FindTool(const std::string& name) const;
    std::vector<MCPTool*> GetToolList(const std::string& category = "") const;
    std::vector<std::string> GetCategories() const;
    size_t GetToolCount() const { return m_Tools.size(); }

private:
    std::unordered_map<std::string, std::unique_ptr<MCPTool>> m_Tools;
};

} // namespace MCP
} // namespace Prisma
