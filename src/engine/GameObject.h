#pragma once
#include "Transform.h"
#include <memory>
#include <string>
#include <vector>
namespace Prisma {


class GameObject
{
public:
    std::string name;
    
    GameObject(std::string name) : name(std::move(name)) {
        m_Transform = AddComponent<Transform>();
    }
    
    GameObject() : GameObject("GameObject") {}

    [[nodiscard]] std::shared_ptr<Transform> GetTransform() { return m_Transform; }
    
    template<typename T>
    std::shared_ptr<T> AddComponent() {
        auto component = std::make_shared<T>();
        m_Components.push_back(component);
        component->SetOwner(this);
        component->Initialize();
        return component;
    }

    template <typename T>
    std::shared_ptr<T> GetComponent() {
        // FIXME: This is slow! Don't call this every frame.
        for (auto& comp : m_Components) {
            auto casted = std::dynamic_pointer_cast<T>(comp);
            if (casted) return casted;
        }
        return nullptr;
    }

    void Update(Timestep ts) {
        for (auto& comp : m_Components) {
            comp->Update(ts);
        }
    }


private:
    std::shared_ptr<Transform> m_Transform;
    std::vector<std::shared_ptr<Component>> m_Components;
};
}