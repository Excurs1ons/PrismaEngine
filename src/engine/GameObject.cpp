#include "GameObject.h"
#include "Component.h"
#include "Transform.h"

GameObject::GameObject(std::string name, std::unique_ptr<Transform> transform) {
    this->name = name;
    if (transform) {
        this->m_transform = std::move(transform);
    } else {
        this->m_transform = std::make_unique<Transform>();
        m_transform->Owner(this);
        m_transform->Initialize();
    }
}