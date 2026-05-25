#pragma once

#include "Export.h"
#include "core/Node.h"
#include "core/Timestep.h"
#include "math/MathTypes.h"
#include <string>
#include <memory>
#include <cstdint>

namespace Prisma {

// ═══════════════════════════════════════════════════════════════════
// ComponentId — 整型组件类型标识（替代 std::type_index）
// ═══════════════════════════════════════════════════════════════════

using ComponentId = uint32_t;

namespace Detail {
    /// 自增 ID 生成器（线程安全 via C++11 magic static）
    inline ComponentId GenerateComponentId() noexcept {
        static ComponentId s_nextId = 1; // 0 保留为无效 ID
        return s_nextId++;
    }
} // namespace Detail

/// 每个 T 获得唯一的 ComponentId（基于模板参数特化的 magic static）
template<typename T>
ComponentId GetComponentTypeId() noexcept {
    static const ComponentId id = Detail::GenerateComponentId();
    return id;
}

// ═══════════════════════════════════════════════════════════════════

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

    // ── ComponentId（整型 ID，用于 ECS 快速查询） ──
    /// 返回该组件实例的运行时类型 ID
    virtual ComponentId GetComponentId() const = 0;

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

    // 启用/禁用
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

    // ── 组件元数据 ──
    std::string name;        ///< 组件名称（用户可读）
    std::string description; ///< 组件描述

protected:
    Node m_ownerNode;
    Scene* m_ownerScene = nullptr;
    bool m_Enabled = true;
};

} // namespace Prisma
