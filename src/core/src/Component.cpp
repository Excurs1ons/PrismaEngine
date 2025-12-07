#include "Component.h"
#include "RendererComponent.h"

// Provide default empty implementations for virtual functions declared in headers

void Component::Update(float /*deltaTime*/) {
    // Default no-op update
}

void RendererComponent::Render(RenderCommandContext* /*context*/) {
    // Default no-op render
}

void RendererComponent::Update(float deltaTime) {
    // By default, call base Component update behavior
    Component::Update(deltaTime);
}

RendererComponent::~RendererComponent() {}

void RendererComponent::Initialize() {}

void RendererComponent::Shutdown() {}
