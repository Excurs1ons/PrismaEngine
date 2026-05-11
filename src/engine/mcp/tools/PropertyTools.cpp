#include "PropertyTools.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "scene/GameObject.h"
#include "transform/Transform.h"
#include "graphic/MeshRenderer.h"

namespace Prisma {
namespace MCP {

PropertyGetTool::PropertyGetTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json PropertyGetTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_index", {{"type", "integer"}, {"description", "Entity index in scene (0-based)"}}},
            {"component", {{"type", "string"}, {"description", "Component name: transform, mesh_renderer"}}}
        }},
        {"required", {"entity_index", "component"}}
    };
}

nlohmann::json PropertyGetTool::Execute(const nlohmann::json& args) {
    auto entityIdx = args["entity_index"].get<size_t>();
    auto component = args["component"].get<std::string>();

    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    const auto& allEntities = scene->GetGameObjects();
    if (entityIdx >= allEntities.size()) {
        return {{"error", "Entity index out of range"}};
    }

    auto entity = allEntities[entityIdx];

    if (component == "transform") {
        auto* transform = entity->GetTransform().get();
        if (!transform) return {{"error", "Transform not found"}};

        auto pos = transform->GetPosition();
        auto rot = transform->GetRotation();
        auto scale = transform->GetScale();

        nlohmann::json result;
        result["entity_index"] = entityIdx;
        result["component"] = "transform";
        result["position"] = {pos.x, pos.y, pos.z};
        result["rotation"] = {rot.x, rot.y, rot.z, rot.w};
        result["scale"] = {scale.x, scale.y, scale.z};
        return result;
    } else if (component == "mesh_renderer") {
        auto mr = entity->GetComponent<Graphic::MeshRenderer>();
        if (!mr) return {{"error", "MeshRenderer not found"}, {"entity_index", entityIdx}};

        nlohmann::json result;
        result["entity_index"] = entityIdx;
        result["component"] = "mesh_renderer";

        auto mat = mr->GetMaterial();
        if (mat) {
            result["material"] = {{"name", mat->getName()}};
        } else {
            result["material"] = nlohmann::json::object();
        }
        return result;
    }

    return {{"error", "Unknown component type"}, {"component", component}};
}

PropertySetTool::PropertySetTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json PropertySetTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_index", {{"type", "integer"}, {"description", "Entity index in scene (0-based)"}}},
            {"component", {{"type", "string"}, {"description", "Component name: transform, mesh_renderer"}}},
            {"property", {{"type", "string"}, {"description", "Property name: position, rotation, scale"}}},
            {"value", {{"type", "array"}, {"description", "Property value as array [x,y,z] or [x,y,z,w]"}}}
        }},
        {"required", {"entity_index", "component", "property", "value"}}
    };
}

nlohmann::json PropertySetTool::Execute(const nlohmann::json& args) {
    auto entityIdx = args["entity_index"].get<size_t>();
    auto component = args["component"].get<std::string>();
    auto property = args["property"].get<std::string>();
    auto value = args["value"];

    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
    if (!scene) return {{"error", "No active scene"}};

    const auto& allEntities = scene->GetGameObjects();
    if (entityIdx >= allEntities.size()) {
        return {{"error", "Entity index out of range"}};
    }

    auto entity = allEntities[entityIdx];

    if (component == "transform") {
        auto* transform = entity->GetTransform().get();
        if (!transform) return {{"error", "Transform not found"}};

        auto makeVec3 = [](const nlohmann::json& v) -> Vector3 {
            return Vector3(v[0].get<float>(), v[1].get<float>(), v[2].get<float>());
        };

        if (property == "position") {
            transform->SetPosition(makeVec3(value));
        } else if (property == "scale") {
            transform->SetScale(makeVec3(value));
        } else if (property == "rotation") {
            if (value.size() == 4) {
                transform->SetRotation(Quaternion(value[0].get<float>(), value[1].get<float>(),
                                                   value[2].get<float>(), value[3].get<float>()));
            } else if (value.size() == 3) {
                transform->SetRotation(makeVec3(value));
            } else {
                return {{"error", "Invalid rotation value"}};
            }
        } else {
            return {{"error", "Unknown transform property"}, {"property", property}};
        }

        nlohmann::json result;
        result["success"] = true;
        result["entity_index"] = entityIdx;
        result["component"] = "transform";
        result["property"] = property;
        result["value"] = value;
        return result;
    }

    return {{"error", "Unknown component type"}, {"component", component}};
}

} // namespace MCP
} // namespace Prisma