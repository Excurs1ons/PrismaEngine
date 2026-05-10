#include "ECSTools.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "scene/GameObject.h"
#include "transform/Transform.h"
#include "graphic/SpriteRenderer.h"
#include <set>

namespace Prisma {
namespace MCP {

ECSComponentListTool::ECSComponentListTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json ECSComponentListTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_index", {{"type", "integer"}, {"description", "Entity index in scene (0-based)"}}}
        }},
        {"required", {"entity_index"}}
    };
}

nlohmann::json ECSComponentListTool::Execute(const nlohmann::json& args) {
    auto entityIdx = args["entity_index"].get<size_t>();
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    const auto& allEntities = scene->GetGameObjects();
    if (entityIdx >= allEntities.size()) {
        return {{"error", "Entity index out of range"}};
    }

    auto entity = allEntities[entityIdx];
    nlohmann::json comps = nlohmann::json::array();
    comps.push_back("Transform");  // All entities have Transform

    // Try to detect common component types via casts
    if (entity->GetComponent<Graphic::SpriteRenderer>())
        comps.push_back("SpriteRenderer");

    return {{"entity_index", entityIdx}, {"components", comps}};
}

ECSComponentGetTool::ECSComponentGetTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json ECSComponentGetTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_index", {{"type", "integer"}}},
            {"component_type", {{"type", "string"}}}
        }},
        {"required", {"entity_index", "component_type"}}
    };
}

nlohmann::json ECSComponentGetTool::Execute(const nlohmann::json& args) {
    auto entityIdx = args["entity_index"].get<size_t>();
    auto compType = args["component_type"].get<std::string>();

    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    const auto& allEntities = scene->GetGameObjects();
    if (entityIdx >= allEntities.size()) {
        return {{"error", "Entity index out of range"}};
    }

    auto entity = allEntities[entityIdx];
    nlohmann::json data;

    if (compType == "Transform") {
        auto* transform = entity->GetTransform().get();
        if (!transform) return {{"error", "Transform not found"}};
        auto pos = transform->GetPosition();
        auto rot = transform->GetRotation();
        auto scale = transform->GetScale();
        data = {
            {"position", {pos.x, pos.y, pos.z}},
            {"rotation", {rot.x, rot.y, rot.z, rot.w}},
            {"scale", {scale.x, scale.y, scale.z}}
        };
    } else if (compType == "SpriteRenderer") {
        auto* sr = entity->GetComponent<Graphic::SpriteRenderer>().get();
        if (!sr) return {{"error", "SpriteRenderer not found"}};
        data = {{"type", "SpriteRenderer"}};
    } else {
        return {{"error", "Unknown component type"}, {"component_type", compType}};
    }

    return {{"entity_index", entityIdx}, {"component_type", compType}, {"data", data}};
}

} // namespace MCP
} // namespace Prisma
