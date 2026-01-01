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
    
    //get属性
    [[nodiscard]] Transform* GetTransform() const { return transform.get(); }
    
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
    
	/// @brief 获取组件
    template<typename T>
    T* GetComponent() {
        for (auto& comp : components) {
            if (auto* casted = dynamic_cast<T*>(comp.get())) {
                return casted;
            }
        }
        return nullptr;
    }

    void Update(float deltaTime) {
        for (auto& comp : components) {
            comp->Update(deltaTime);
        }
    }

    void AddComponent(const std::shared_ptr<Component>& component) {
        component->SetOwner(this);
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
    std::unique_ptr<Transform> transform;
    std::vector<std::shared_ptr<Component>> components;
};
}