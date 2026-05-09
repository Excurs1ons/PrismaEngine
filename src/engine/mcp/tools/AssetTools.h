#pragma once
#include "../MCPTool.h"

namespace Prisma {
namespace MCP {

class AssetListTool : public MCPTool {
public:
    AssetListTool();
    std::string_view GetName() const override { return "asset_list"; }
    std::string_view GetDescription() const override {
        return "List assets with optional type filter and pagination.";
    }
    std::string_view GetCategory() const override { return "asset"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
};

class AssetGetInfoTool : public MCPTool {
public:
    AssetGetInfoTool();
    std::string_view GetName() const override { return "asset_get_info"; }
    std::string_view GetDescription() const override { return "Get detailed metadata for an asset."; }
    std::string_view GetCategory() const override { return "asset"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
};

} // namespace MCP
} // namespace Prisma
