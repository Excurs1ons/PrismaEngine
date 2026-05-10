#include "AssetTools.h"

namespace Prisma {
namespace MCP {

AssetListTool::AssetListTool() = default;

nlohmann::json AssetListTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"type", {{"type", "string"}, {"description", "Filter: texture, mesh, shader, audio, scene"}}},
            {"page", {{"type", "integer"}, {"description", "Page number"}}},
            {"per_page", {{"type", "integer"}, {"description", "Items per page (default 20)"}}}
        }}
    };
}

nlohmann::json AssetListTool::Execute(const nlohmann::json& /*args*/) {
    // TODO: Integrate with AssetDatabase
    return {{"assets", nlohmann::json::array()}, {"total", 0}};
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

nlohmann::json AssetGetInfoTool::Execute(const nlohmann::json& /*args*/) {
    return {{"error", "Not implemented"}};
}

} // namespace MCP
} // namespace Prisma
