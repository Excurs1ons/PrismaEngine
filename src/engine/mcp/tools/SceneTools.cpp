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

glz::json_t SceneHierarchyTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"fields", {{"type", "array", "items", {{"type", "string"}}},
                        {"description", "Optional: filter response fields. E.g. [\"name\"]"}}}
        }}
    };
}

glz::json_t SceneHierarchyTool::Execute(const glz::json_t& args) {
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) {
        return {{"error", "No active scene"}, {"entities", std::vector<glz::json_t>{}}};
    }

    std::vector<std::string> fields;
    if (args.get_object().contains("fields")) {
        auto fields_json = args["fields"].get_array();
        for (const auto& f : fields_json) {
            fields.push_back(f.get_string());
        }
    } else {
        fields = {"name"};
    }
    const auto& allEntities = scene->GetGameObjects();

    glz::json_t entities = std::vector<glz::json_t>{};
    uint32_t idCounter = 1;
    for (const auto& entity : allEntities) {
        glz::json_t e;
        if (fields.empty() || std::find(fields.begin(), fields.end(), "name") != fields.end())
            e["name"] = entity->name;
        e["id"] = static_cast<double>(idCounter++);
        entities.get_array().push_back(std::move(e));
    }

    return {{"scene_name", scene->GetName()}, {"entity_count", static_cast<double>(allEntities.size())}, {"entities", std::move(entities)}};
}

// ---- SceneEntityTool ----

SceneEntityTool::SceneEntityTool(Engine* engine) : m_Engine(engine) {}

glz::json_t SceneEntityTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_index", {{"type", "integer"}, {"description", "Entity index in scene (0-based)"}}},
            {"fields", {{"type", "array", "items", {{"type", "string"}}}}}
        }},
        {"required", {"entity_index"}}
    };
}

glz::json_t SceneEntityTool::Execute(const glz::json_t& args) {
    auto entityIdx = static_cast<size_t>(args["entity_index"].get_number());
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    const auto& allEntities = scene->GetGameObjects();
    if (entityIdx >= allEntities.size()) {
        return {{"error", "Entity index out of range"}, {"max_index", static_cast<double>(allEntities.size() - 1)}};
    }

    auto entity = allEntities[entityIdx];
    std::vector<std::string> fields;
    if (args.get_object().contains("fields")) {
        auto fields_json = args["fields"].get_array();
        for (const auto& f : fields_json) {
            fields.push_back(f.get_string());
        }
    }
    bool allFields = fields.empty();

    glz::json_t result;
    result["entity_index"] = static_cast<double>(entityIdx);

    if (allFields || std::find(fields.begin(), fields.end(), "name") != fields.end())
        result["name"] = entity->name;

    if (allFields || std::find(fields.begin(), fields.end(), "transform") != fields.end()) {
        auto* transform = entity->GetTransform().get();
        if (transform) {
            result["position"] = std::vector<glz::json_t>{
                transform->GetPosition().x, 
                transform->GetPosition().y, 
                transform->GetPosition().z
            };
        }
    }

    return result;
}

// ---- SceneCreateEntityTool ----

SceneCreateEntityTool::SceneCreateEntityTool(Engine* engine) : m_Engine(engine) {}

glz::json_t SceneCreateEntityTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"name", {{"type", "string"}}}
        }},
        {"required", {"name"}}
    };
}

glz::json_t SceneCreateEntityTool::Execute(const glz::json_t& args) {
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    auto name = args["name"].get_string();
    auto entity = std::make_shared<GameObject>(name);
    entity->Initialize();
    scene->AddGameObject(entity);

    return {{"name", name}};
}

// ---- SceneDeleteEntityTool ----

SceneDeleteEntityTool::SceneDeleteEntityTool(Engine* engine) : m_Engine(engine) {}

glz::json_t SceneDeleteEntityTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_index", {{"type", "integer"}, {"description", "Entity index in scene (0-based)"}}}
        }},
        {"required", {"entity_index"}}
    };
}

glz::json_t SceneDeleteEntityTool::Execute(const glz::json_t& args) {
    auto entityIdx = static_cast<size_t>(args["entity_index"].get_number());
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
