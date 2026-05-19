#include "Component.h"
#include "scene/Scene.h"
#include "transform/Transform.h"
#include "transform/Camera.h"
#include "ui/TextRendererComponent.h"

namespace Prisma {

// ── GetSiblingComponent 模板定义 ──

template<typename T>
std::shared_ptr<T> Component::GetSiblingComponent() const {
    if (!m_ownerScene) return nullptr;
    for (auto& comp : m_ownerScene->GetComponents(m_ownerNode)) {
        auto result = std::dynamic_pointer_cast<T>(comp);
        if (result) return result;
    }
    return nullptr;
}

// 显式实例化
template std::shared_ptr<Transform> Component::GetSiblingComponent<Transform>() const;
template std::shared_ptr<Graphic::Camera> Component::GetSiblingComponent<Graphic::Camera>() const;
template std::shared_ptr<TextRendererComponent> Component::GetSiblingComponent<TextRendererComponent>() const;

// ── GetTransform ──

std::shared_ptr<Transform> Component::GetTransform() const {
    return GetSiblingComponent<Transform>();
}

// ── GetNodeName ──

std::string Component::GetNodeName() const {
    if (m_ownerScene) {
        return m_ownerScene->GetNodeName(m_ownerNode);
    }
    return "";
}

} // namespace Prisma
