#include "ECSTools.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "transform/Transform.h"
#include "graphic/SpriteRenderer.h"
#include <set>

namespace Prisma {
namespace MCP {

ECSComponentListTool::ECSComponentListTool(Engine* engine) : m_Engine(engine) {}

glz::json_t ECSComponentListTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_index", {{"type", "integer"}, {"description", "Entity index in scene (0-based)"}}}
        }},
        {"required", {"entity_index"}}
    };
}

glz::json_t ECSComponentListTool::Execute(const glz::json_t& args) {
    auto entityIdx = static_cast<size_t>(args["entity_index"].get_number());
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    const auto& allEntities = scene->GetNodes();
    if (entityIdx >= allEntities.size()) {
        return {{"error", "Entity index out of range"}};
    }

    auto entity = allEntities[entityIdx];
    glz::json_t comps = std::vector<glz::json_t>{};
    comps.get_array().push_back("Transform");  // All entities have Transform

    // Try to detect common component types via casts
    if (scene->GetComponent<Graphic::SpriteRenderer>(entity))
        comps.get_array().push_back("SpriteRenderer");

    return {{"entity_index", static_cast<double>(entityIdx)}, {"components", std::move(comps)}};
}

ECSComponentGetTool::ECSComponentGetTool(Engine* engine) : m_Engine(engine) {}

glz::json_t ECSComponentGetTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_index", {{"type", "integer"}}},
            {"component_type", {{"type", "string"}}}
        }},
        {"required", {"entity_index", "component_type"}}
    };
}

glz::json_t ECSComponentGetTool::Execute(const glz::json_t& args) {
    auto entityIdx = static_cast<size_t>(args["entity_index"].get_number());
    auto compType = args["component_type"].get_string();

    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    const auto& allEntities = scene->GetNodes();
    if (entityIdx >= allEntities.size()) {
        return {{"error", "Entity index out of range"}};
    }

    auto entity = allEntities[entityIdx];
    glz::json_t data;

    if (compType == "Transform") {
        auto* transform = scene->GetComponent<Transform>(entity).get();
        if (!transform) return {{"error", "Transform not found"}};
        auto pos = transform->GetPosition();
        auto rot = transform->GetRotation();
        auto scale = transform->GetScale();
        data = glz::json_t::object_t{
            {"position", glz::json_t::array_t{static_cast<double>(pos.x), static_cast<double>(pos.y), static_cast<double>(pos.z)}},
            {"rotation", glz::json_t::array_t{static_cast<double>(rot.x), static_cast<double>(rot.y), static_cast<double>(rot.z), static_cast<double>(rot.w)}},
            {"scale", glz::json_t::array_t{static_cast<double>(scale.x), static_cast<double>(scale.y), static_cast<double>(scale.z)}}
        };
    } else if (compType == "SpriteRenderer") {
        auto* sr = scene->GetComponent<Graphic::SpriteRenderer>(entity).get();
        if (!sr) return {{"error", "SpriteRenderer not found"}};
        data = glz::json_t::object_t{{"type", "SpriteRenderer"}};
    } else {
        return {{"error", "Unknown component type"}, {"component_type", compType}};
    }

    return {{"entity_index", static_cast<double>(entityIdx)}, {"component_type", compType}, {"data", std::move(data)}};
}

ECSComponentSetTool::ECSComponentSetTool(Engine* engine) : m_Engine(engine) {}

glz::json_t ECSComponentSetTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_index", {{"type", "integer"}}},
            {"component_type", {{"type", "string"}}},
            {"data", {{"type", "object"}}}
        }},
        {"required", {"entity_index", "component_type", "data"}}
    };
}

glz::json_t ECSComponentSetTool::Execute(const glz::json_t& args) {
    auto entityIdx = static_cast<size_t>(args["entity_index"].get_number());
    auto compType = args["component_type"].get_string();
    auto data = args["data"];

    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    const auto& allEntities = scene->GetNodes();
    if (entityIdx >= allEntities.size()) {
        return {{"error", "Entity index out of range"}};
    }

    auto entity = allEntities[entityIdx];

    if (compType == "Transform") {
        auto transform = scene->GetComponent<Transform>(entity);
        m_Engine->SubmitToMainThread([transform, data]() {
            if (data.get_object().contains("position")) {
                auto p = data["position"];
                transform->SetPosition({static_cast<float>(p[0].get_number()), 
                                        static_cast<float>(p[1].get_number()), 
                                        static_cast<float>(p[2].get_number())});
            }
            if (data.get_object().contains("scale")) {
                auto s = data["scale"];
                transform->SetScale({static_cast<float>(s[0].get_number()), 
                                     static_cast<float>(s[1].get_number()), 
                                     static_cast<float>(s[2].get_number())});
            }
        });
        return {{"success", true}, {"deferred", true}};
    } else if (compType == "Node") {
        if (data.get_object().contains("name")) {
            std::string name = data["name"].get_string();
            m_Engine->SubmitToMainThread([scene, entity, name]() {
                scene->SetNodeName(entity, name);
            });
        }
        return {{"success", true}, {"deferred", true}};
    }

    return {{"error", "Setting for component type not implemented yet"}, {"component_type", compType}};
}

} // namespace MCP
} // namespace Prisma
