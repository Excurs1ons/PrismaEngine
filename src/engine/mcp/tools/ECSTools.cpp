#include "ECSTools.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "scene/GameObject.h"
#include <set>

namespace Prisma {
namespace MCP {

ECSComponentListTool::ECSComponentListTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json ECSComponentListTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_id", {{"type", "integer"}, {"description", "Entity ID"}}}
        }},
        {"required", {"entity_id"}}
    };
}

nlohmann::json ECSComponentListTool::Execute(const nlohmann::json& args) {
    auto entityId = args["entity_id"].get<uint32_t>();
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    auto entity = scene->FindEntity(entityId);
    if (!entity) return {{"error", "Entity not found"}, {"entity_id", entityId}};

    auto components = entity->GetComponentTypes();
    return {{"entity_id", entityId}, {"components", components}};
}

ECSComponentGetTool::ECSComponentGetTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json ECSComponentGetTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_id", {{"type", "integer"}}},
            {"component_type", {{"type", "string"}}}
        }},
        {"required", {"entity_id", "component_type"}}
    };
}

nlohmann::json ECSComponentGetTool::Execute(const nlohmann::json& args) {
    auto entityId = args["entity_id"].get<uint32_t>();
    auto compType = args["component_type"].get<std::string>();

    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    auto entity = scene->FindEntity(entityId);
    if (!entity) return {{"error", "Entity not found"}, {"entity_id", entityId}};

    auto compData = entity->GetComponentData(compType);
    if (compData.is_null()) {
        return {{"error", "Component not found"}, {"entity_id", entityId}, {"component_type", compType}};
    }

    return {
        {"entity_id", entityId},
        {"component_type", compType},
        {"data", compData}
    };
}

} // namespace MCP
} // namespace Prisma
