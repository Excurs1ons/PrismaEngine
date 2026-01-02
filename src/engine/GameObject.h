#pragma once
#include "Transform.h"
#include <memory>
#include <string>
#include <vector>
namespace PrismaEngine {


class GameObject
{
public:
    std::string name;
    GameObject(std::string name, std::unique_ptr<Transform> transform = nullptr);
    GameObject();
    //get属性
    [[nodiscard]] std::shared_ptr<Transform> GetTransform() { return transform; }
    
	/// @brief 添加组件
    template<typename T>
    T* AddComponent() {
        auto component = std::make_unique<T>();
        T* ptr = component.get();
        components.push_back(std::unique_ptr<Component>(component.release()));
        ptr->Owner(this);
        ptr->Initialize();
        return ptr;
    }

    void Update(float deltaTime) {
        for (auto& comp : components) {
            comp->Update(deltaTime);
        }
    }

    void AddComponent(const std::shared_ptr<Component>& component) {
        component->SetOwner(this);
        component->Initialize();
        components.push_back(component);
    }

    template <typename T>
    std::shared_ptr<T> GetComponent() {
        for (auto& comp : components) {
            if (auto casted = std::dynamic_pointer_cast<T>(comp)) {
                return casted;
            }
        }
        return nullptr;
    }

    void update(float deltaTime) {
        for (auto& comp : components) {
            comp->Update(deltaTime);
        }
    }

    Vector3 position{0.0f, 0.0f, 0.0f};
    Vector3 rotation{0.0f, 0.0f, 0.0f}; // Euler angles in degrees
    Vector3 scale{1.0f, 1.0f, 1.0f};

    [[nodiscard]] Matrix4 GetTransformMatrix() const {
        auto model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotation.x), Vector3(1, 0, 0));
        model = glm::rotate(model, glm::radians(rotation.y), Vector3(0, 1, 0));
        model = glm::rotate(model, glm::radians(rotation.z), Vector3(0, 0, 1));
        model = glm::scale(model, scale);
        return model;
    }
private:
    std::shared_ptr<Transform> transform;
    std::vector<std::shared_ptr<Component>> components;
};
}