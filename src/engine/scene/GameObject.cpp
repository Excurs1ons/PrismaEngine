#include "GameObject.h"
#include "Component.h"
#include "Transform.h"

namespace Prisma {

GameObject::GameObject() : name("GameObject") {
    m_Transform = std::make_shared<Transform>();
}

GameObject::GameObject(std::string name) : name(std::move(name)) {
    m_Transform = std::make_shared<Transform>();
}

GameObject::~GameObject() {
    Shutdown();
}

void GameObject::Initialize() {
    if (m_Transform) {
        m_Transform->SetOwner(this);
        m_Transform->Initialize();
    }
}

void GameObject::Update(Timestep ts) {
    if (m_Transform) {
        m_Transform->Update(ts);
    }
    for (auto& comp : m_Components) {
        comp->Update(ts);
    }
}

void GameObject::Shutdown() {
    for (auto& comp : m_Components) {
        comp->Shutdown();
    }
    m_Components.clear();
    if (m_Transform) {
        m_Transform->Shutdown();
    }
}

} // namespace Prisma
