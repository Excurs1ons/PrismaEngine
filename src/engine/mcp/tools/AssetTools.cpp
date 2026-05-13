#include "AssetTools.h"
#include "core/AssetDatabase.h"

namespace Prisma {
namespace MCP {

AssetListTool::AssetListTool() = default;

nlohmann::json AssetListTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"type", {{"type", "string"}, {"description", "Filter by file extension (e.g. .png, .scene)"}}},
            {"page", {{"type", "integer"}}},
            {"per_page", {{"type", "integer"}}}
        }}
    };
}

nlohmann::json AssetListTool::Execute(const nlohmann::json& args) {
    std::string typeFilter = args.value("type", "");
    auto& db = AssetDatabase::Get();
    const auto& allAssets = db.GetAllMetadata();

    nlohmann::json assets = nlohmann::json::array();
    for (const auto& [path, meta] : allAssets) {
        if (!typeFilter.empty() && meta.type != typeFilter) continue;

        assets.push_back({
            {"path", meta.path},
            {"type", meta.type},
            {"guid", meta.guid.ToString()}
        });
    }

    return {{"assets", assets}, {"total", assets.size()}};
}

AssetGetInfoTool::AssetGetInfoTool() = default;

nlohmann::json AssetGetInfoTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"path", {{"type", "string"}, {"description", "Asset path"}}}
        }},
        {"required", {"path"}}
    };
}

nlohmann::json AssetGetInfoTool::Execute(const nlohmann::json& args) {
    std::string path = args["path"].get<std::string>();
    auto* meta = AssetDatabase::Get().GetMetadata(path);
    if (!meta) return {{"error", "Asset not found"}};

    nlohmann::json j;
    meta->ToJson(j);
    return {{"metadata", j}};
}

} // namespace MCP
} // namespace Prisma
