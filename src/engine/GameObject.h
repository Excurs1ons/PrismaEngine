#pragma once
#include "Transform.h"
#include <memory>
#include <string>
#include <vector>

class GameObject
{
public:
    std::string name;
    GameObject(std::string name, std::unique_ptr<Transform> transform = nullptr);
    
    //get属性
    Transform* transform() const { return m_transform.get(); }
    
	/// @brief 添加组件
    template<typename T>
    T* AddComponent() {
        auto component = std::make_unique<T>();
        T* ptr = component.get();
        m_components.push_back(std::unique_ptr<Component>(component.release()));
        ptr->Owner(this);
        ptr->Initialize();
        return ptr;
    }
    
	/// @brief 获取组件
    template<typename T>
    T* GetComponent() {
        for (auto& comp : m_components) {
            if (auto* casted = dynamic_cast<T*>(comp.get())) {
                return casted;
            }
        }
        return nullptr;
    }

    void Update(float deltaTime) {
        for (auto& comp : m_components) {
            comp->Update(deltaTime);
        }
    }
    
private:
    std::unique_ptr<Transform> m_transform;
    std::vector<std::unique_ptr<Component>> m_components;
};