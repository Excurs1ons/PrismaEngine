#include "Component.h"

// Provide default empty implementations for virtual functions declared in headers

GameObject* Component::gameObject() {
    return m_owner;
}

void Component::Update(float /*deltaTime*/) {
    // Default no-op update
}

Component::Component() {}
