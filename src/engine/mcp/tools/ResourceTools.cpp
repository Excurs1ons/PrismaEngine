#include "ResourceTools.h"
#include "app/Engine.h"
#include "core/AssetManager.h"

namespace Prisma {
namespace MCP {

// ---- ResourceListTool ----

ResourceListTool::ResourceListTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json ResourceListTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"type_filter", {{"type", "string"}, {"description", "Filter: texture, mesh, shader, audio, scene, or all"}}}
        }}
    };
}

nlohmann::json ResourceListTool::Execute(const nlohmann::json& args) {
    auto* assetManager = m_Engine->GetAssetManager();
    if (!assetManager) {
        return {{"error", "AssetManager not available"}};
    }

    // Get type filter if provided
    std::string typeFilter = args.value("type_filter", "all");

    // For now, return empty list since cache iteration is internal
    // TODO: Expose cache iteration in AssetManager
    nlohmann::json result;
    result["count"] = 0;
    result["resources"] = nlohmann::json::array();
    result["type_filter"] = typeFilter;
    result["note"] = "Resource cache iteration not yet exposed. Use resource_load to verify specific resources.";

    return result;
}

// ---- ResourceLoadTool ----

ResourceLoadTool::ResourceLoadTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json ResourceLoadTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"path", {{"type", "string"}, {"description", "Absolute path to the resource"}}}
        }},
        {"required", {"path"}}
    };
}

nlohmann::json ResourceLoadTool::Execute(const nlohmann::json& args) {
    auto* assetManager = m_Engine->GetAssetManager();
    if (!assetManager) {
        return {{"error", "AssetManager not available"}};
    }

    std::string path = args["path"].get<std::string>();
    if (path.empty()) {
        return {{"error", "Path cannot be empty"}};
    }

    // Try to find the resource path
    auto fullPath = assetManager->FindResource(path);
    if (!fullPath) {
        return {{"error", "Resource not found in search paths: " + path}};
    }

    nlohmann::json result;
    result["path"] = path;
    result["full_path"] = fullPath->string();
    result["loaded"] = true;
    result["note"] = "Resource path verified. Full loading requires type-specific API.";

    return result;
}

// ---- ResourceUnloadTool ----

ResourceUnloadTool::ResourceUnloadTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json ResourceUnloadTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"path", {{"type", "string"}, {"description", "Path to the resource to unload"}}}
        }},
        {"required", {"path"}}
    };
}

nlohmann::json ResourceUnloadTool::Execute(const nlohmann::json& args) {
    auto* assetManager = m_Engine->GetAssetManager();
    if (!assetManager) {
        return {{"error", "AssetManager not available"}};
    }

    std::string path = args["path"].get<std::string>();
    if (path.empty()) {
        return {{"error", "Path cannot be empty"}};
    }

    assetManager->Unload(path);

    nlohmann::json result;
    result["path"] = path;
    result["unloaded"] = true;

    return result;
}

} // namespace MCP
} // namespace Prisma