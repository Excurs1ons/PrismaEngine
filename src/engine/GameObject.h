#pragma once
#include "Transform.h"
#include <memory>
#include <string>
#include <vector>

namespace Prisma {

class Component;

class GameObject : public std::enable_shared_from_this<GameObject>
{
public:
    std::string name;
    
    GameObject();
    GameObject(std::string name);
    virtual ~GameObject();

    void Initialize();
    void Update(Timestep ts);
    void Shutdown();

    template<typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Args&&... args) {
        auto comp = std::make_shared<T>(std::forward<Args>(args)...);
        comp->SetOwner(shared_from_this().get());
        comp->Initialize();
        m_Components.push_back(comp);
        return comp;
    }

    template<typename T>
    std::shared_ptr<T> GetComponent() {
        for (auto& comp : m_Components) {
            auto result = std::dynamic_pointer_cast<T>(comp);
            if (result) return result;
        }
        return nullptr;
    }

    std::shared_ptr<Transform> GetTransform() const { return m_Transform; }

private:
    std::shared_ptr<Transform> m_Transform;
    std::vector<std::shared_ptr<Component>> m_Components;
};

} // namespace Prisma
