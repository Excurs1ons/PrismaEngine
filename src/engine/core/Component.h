#pragma once

#include "Export.h"
#include "core/Node.h"
#include "core/Timestep.h"
#include "math/MathTypes.h"
#include <string>
#include <memory>

namespace Prisma {

class Scene;
class Transform;

class ENGINE_API Component {
public:
    virtual ~Component() = default;
    virtual void Initialize(){};
    virtual void Update([[maybe_unused]] Timestep ts) {}
    virtual void Shutdown(){};

    // 返回组件类型名称（用于序列化），默认返回 nullptr 表示不可序列化
    virtual const char* GetComponentTypeName() const { return nullptr; }

    // ── Node 所有权（取代旧的 GameObject 所有权） ──
    void SetOwnerNode(Node node, Scene* scene) { m_ownerNode = node; m_ownerScene = scene; }
    void SetOwnerNodeOnly(Node node) { m_ownerNode = node; }
    [[nodiscard]] Node GetOwnerNode() const { return m_ownerNode; }
    [[nodiscard]] Scene* GetOwnerScene() const { return m_ownerScene; }

    // 便捷：获取同 Node 的兄弟组件
    // （定义在 Component.cpp 中显式实例化，避免 Scene 不完整问题）
    template<typename T>
    std::shared_ptr<T> GetSiblingComponent() const;

    // 便捷：获取同 Node 的 Transform
    std::shared_ptr<Transform> GetTransform() const;

    // 便捷：获取所属 Node 名称
    std::string GetNodeName() const;

protected:
    Node m_ownerNode;
    Scene* m_ownerScene = nullptr;
};

} // namespace Prisma
