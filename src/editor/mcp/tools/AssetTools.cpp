#include "AssetTools.h"
#include "core/AssetDatabase.h"

namespace Prisma {
namespace MCP {

AssetListTool::AssetListTool() = default;

glz::json_t AssetListTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"type", {{"type", "string"}, {"description", "Filter by file extension (e.g. .png, .scene)"}}},
            {"page", {{"type", "integer"}}},
            {"per_page", {{"type", "integer"}}}
        }}
    };
}

glz::json_t AssetListTool::Execute(const glz::json_t& args) {
    std::string typeFilter = args.get_object().contains("type") ? args["type"].get_string() : "";
    auto& db = AssetDatabase::Get();
    const auto& allAssets = db.GetAllMetadata();

    glz::json_t assets = std::vector<glz::json_t>{};
    for (const auto& [path, meta] : allAssets) {
        if (!typeFilter.empty() && meta.type != typeFilter) continue;

        assets.get_array().push_back(glz::json_t{
            {"path", meta.path},
            {"type", meta.type},
            {"guid", meta.guid.ToString()}
        });
    }

    size_t total = assets.get_array().size();
    return {{"assets", std::move(assets)}, {"total", static_cast<double>(total)}};
}

AssetGetInfoTool::AssetGetInfoTool() = default;

glz::json_t AssetGetInfoTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"path", {{"type", "string"}, {"description", "Asset path"}}}
        }},
        {"required", {"path"}}
    };
}

glz::json_t AssetGetInfoTool::Execute(const glz::json_t& args) {
    std::string path = args["path"].get_string();
    auto* meta = AssetDatabase::Get().GetMetadata(path);
    if (!meta) return {{"error", "Asset not found"}};

    glz::json_t j;
    meta->ToJson(j);
    return {{"metadata", j}};
}

} // namespace MCP
} // namespace Prisma
