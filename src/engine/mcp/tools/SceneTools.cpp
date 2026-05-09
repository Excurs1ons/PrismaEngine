#include "SceneTools.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "scene/GameObject.h"
#include <set>

namespace Prisma {
namespace MCP {

// ---- SceneHierarchyTool ----

SceneHierarchyTool::SceneHierarchyTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json SceneHierarchyTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"fields", {{"type", "array", "items", {{"type", "string"}}},
                        {"description", "Optional: filter response fields. E.g. [\"name\",\"id\"]"}}}
        }}
    };
}

nlohmann::json SceneHierarchyTool::Execute(const nlohmann::json& args) {
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) {
        return {{"error", "No active scene"}, {"entities", nlohmann::json::array()}};
    }

    auto fields = args.value("fields", std::vector<std::string>{"name"});
    const auto& allEntities = scene->GetGameObjects();

    nlohmann::json entities = nlohmann::json::array();
    for (const auto& entity : allEntities) {
        nlohmann::json e;
        e["id"] = (uint64_t)(void*)entity.get();
        if (fields.empty() || std::find(fields.begin(), fields.end(), "name") != fields.end())
            e["name"] = entity->name;
        entities.push_back(std::move(e));
    }

    return {{"scene_name", scene->GetName()}, {"entity_count", allEntities.size()}, {"entities", entities}};
}

// ---- SceneEntityTool ----

SceneEntityTool::SceneEntityTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json SceneEntityTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_id", {{"type", "integer"}, {"description", "Entity ID to query"}}},
            {"fields", {{"type", "array", "items", {{"type", "string"}}}}}
        }},
        {"required", {"entity_id"}}
    };
}

nlohmann::json SceneEntityTool::Execute(const nlohmann::json& args) {
    auto entityId = args["entity_id"].get<uint64_t>();
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    // Find entity by pointer value
    std::shared_ptr<GameObject> found;
    for (const auto& obj : scene->GetGameObjects()) {
        if ((uint64_t)(void*)obj.get() == entityId) {
            found = obj;
            break;
        }
    }
    if (!found) return {{"error", "Entity not found"}, {"entity_id", entityId}};

    auto fields = args.value("fields", std::vector<std::string>{});
    auto fieldSet = std::set<std::string>(fields.begin(), fields.end());
    bool allFields = fields.empty();

    nlohmann::json result;
    result["entity_id"] = entityId;

    if (allFields || fieldSet.count("name"))
        result["name"] = found->name;
    if (allFields || fieldSet.count("position")) {
        auto pos = found->GetTransform()->GetPosition();
        result["position"] = {pos.x, pos.y, pos.z};
    }

    return result;
}

// ---- SceneCreateEntityTool ----

SceneCreateEntityTool::SceneCreateEntityTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json SceneCreateEntityTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"name", {{"type", "string"}}},
            {"parent_id", {{"type", "integer"}, {"description", "Optional parent entity ID"}}}
        }},
        {"required", {"name"}}
    };
}

nlohmann::json SceneCreateEntityTool::Execute(const nlohmann::json& args) {
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    auto name = args["name"].get<std::string>();
    auto entity = std::make_shared<GameObject>(name);
    entity->Initialize();
    scene->AddGameObject(entity);

    return {{"entity_id", (uint64_t)(void*)entity.get()}, {"name", name}};
}

// ---- SceneDeleteEntityTool ----

SceneDeleteEntityTool::SceneDeleteEntityTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json SceneDeleteEntityTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_id", {{"type", "integer"}, {"description", "Entity ID to delete"}}}
        }},
        {"required", {"entity_id"}}
    };
}

nlohmann::json SceneDeleteEntityTool::Execute(const nlohmann::json& args) {
    auto entityId = args["entity_id"].get<uint64_t>();
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    bool deleted = false;
    for (const auto& obj : scene->GetGameObjects()) {
        if ((uint64_t)(void*)obj.get() == entityId) {
            scene->RemoveGameObject(obj.get());
            deleted = true;
            break;
        }
    }
    return {{"deleted", deleted}, {"entity_id", entityId}};
}

} // namespace MCP
} // namespace Prisma
