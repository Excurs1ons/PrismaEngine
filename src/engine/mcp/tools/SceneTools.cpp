#include "SceneTools.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "scene/GameObject.h"
#include <set>
#include <cstdint>

namespace Prisma {
namespace MCP {

// ---- SceneHierarchyTool ----

SceneHierarchyTool::SceneHierarchyTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json SceneHierarchyTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"fields", {{"type", "array", "items", {{"type", "string"}}},
                        {"description", "Optional: filter response fields. E.g. [\"name\"]"}}}
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
    uint32_t idCounter = 1;
    for (const auto& entity : allEntities) {
        nlohmann::json e;
        if (fields.empty() || std::find(fields.begin(), fields.end(), "name") != fields.end())
            e["name"] = entity->name;
        e["id"] = idCounter++;
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
            {"entity_index", {{"type", "integer"}, {"description", "Entity index in scene (0-based)"}}},
            {"fields", {{"type", "array", "items", {{"type", "string"}}}}}
        }},
        {"required", {"entity_index"}}
    };
}

nlohmann::json SceneEntityTool::Execute(const nlohmann::json& args) {
    auto entityIdx = args["entity_index"].get<size_t>();
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    const auto& allEntities = scene->GetGameObjects();
    if (entityIdx >= allEntities.size()) {
        return {{"error", "Entity index out of range"}, {"max_index", allEntities.size() - 1}};
    }

    auto entity = allEntities[entityIdx];
    auto fields = args.value("fields", std::vector<std::string>{});
    bool allFields = fields.empty();

    nlohmann::json result;
    result["entity_index"] = entityIdx;

    if (allFields || std::find(fields.begin(), fields.end(), "name") != fields.end())
        result["name"] = entity->name;

    if (allFields || std::find(fields.begin(), fields.end(), "transform") != fields.end()) {
        auto* transform = entity->GetTransform().get();
        if (transform) {
            result["position"] = {transform->GetPosition().x, transform->GetPosition().y, transform->GetPosition().z};
        }
    }

    return result;
}

// ---- SceneCreateEntityTool ----

SceneCreateEntityTool::SceneCreateEntityTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json SceneCreateEntityTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"name", {{"type", "string"}}}
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

    return {{"name", name}};
}

// ---- SceneDeleteEntityTool ----

SceneDeleteEntityTool::SceneDeleteEntityTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json SceneDeleteEntityTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_index", {{"type", "integer"}, {"description", "Entity index in scene (0-based)"}}}
        }},
        {"required", {"entity_index"}}
    };
}

nlohmann::json SceneDeleteEntityTool::Execute(const nlohmann::json& args) {
    auto entityIdx = args["entity_index"].get<size_t>();
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    const auto& allEntities = scene->GetGameObjects();
    if (entityIdx >= allEntities.size()) {
        return {{"error", "Index out of range"}};
    }

    scene->RemoveGameObject(allEntities[entityIdx]);
    return {{"deleted", true}};
}

} // namespace MCP
} // namespace Prisma
