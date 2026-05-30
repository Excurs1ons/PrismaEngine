#pragma once
#include "core/Node.h"
#include "core/Component.h"
#include "graphic/RenderCommandContext.h"
#include "graphic/RenderComponent.h"
#include "graphic/ICamera.h"
#include "core/Timestep.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>

namespace Prisma {

class Component;
class Transform;

class ENGINE_API Scene
{
public:
    // ── Node 层级数据 ──
    struct SceneNodeData {
        uint32_t parent = UINT32_MAX;     // UINT32_MAX = 根节点
        std::vector<uint32_t> children;
    };

    Scene();
    ~Scene();

    // 基本 Node 管理
    Node CreateNode(const std::string& name = "Node");
    void RemoveNode(Node node);
    void Update(Timestep ts);

    // 名称与状态
    void SetName(const std::string& name) noexcept { m_Name = name; }
    const std::string& GetName() const noexcept { return m_Name; }
    void SetDirty(bool dirty) noexcept { m_IsDirty = dirty; }
    bool IsDirty() const noexcept { return m_IsDirty; }
    const std::vector<Node>& GetNodes() const noexcept { return m_nodes; }

    // ── Node 名称 ──
    std::string GetNodeName(Node node) const;
    void SetNodeName(Node node, const std::string& name);

    // ── 层级 API ──
    void SetParent(Node child, Node parent);
    std::vector<Node> GetChildren(Node node) const;
    Node GetParent(Node node) const;
    std::vector<Node> GetRootNodes() const;
    Matrix4x4 GetWorldTransform(Node node) const;

    // ── 组件 API ──
    template<typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Node node, Args&&... args) {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        auto comp = std::make_shared<T>(std::forward<Args>(args)...);
        comp->SetOwnerNode(node, this);
        comp->Initialize();
        m_nodeComponents[node.handle].push_back(comp);
        m_IsDirty = true;
        return comp;
    }

    template<typename T>
    std::shared_ptr<T> GetComponent(Node node) const {
        auto it = m_nodeComponents.find(node.handle);
        if (it != m_nodeComponents.end()) {
            for (auto& comp : it->second) {
                auto result = std::dynamic_pointer_cast<T>(comp);
                if (result) return result;
            }
        }
        return nullptr;
    }

    const std::vector<std::shared_ptr<Component>>& GetComponents(Node node) const;
    void RemoveComponent(Node node, Component* comp);

    // ── 场景主相机（遍历 Node 查找第一个 Camera 组件） ──
    std::shared_ptr<Prisma::Graphic::ICamera> GetMainCamera();

    // ── 获取场景中所有光源 ──
    std::vector<Prisma::Graphic::Light> GetLights() const;

    // ── ECS 实体管理（替代已废弃的 ECS::World） ──
    using Entity = uint32_t;
    static constexpr Entity INVALID_ENTITY = 0;

    /// 创建一个新实体（本质是创建一个 Node，返回其 handle 作为 Entity）
    Entity CreateEntity(const std::string& name = "Entity") {
        Node node = CreateNode(name);
        return node.handle;
    }

    /// 销毁实体（销毁对应的 Node 及其所有组件）
    void DestroyEntity(Entity entity) {
        RemoveNode(Node(entity));
    }

    /// 遍历所有同时拥有 T1 和 T2 组件的实体，对每个实体调用 callback(component1, component2)
    template<typename T1, typename T2, typename Func>
    void ForEach(Func&& callback) {
        for (const auto& node : m_nodes) {
            auto comp1 = GetComponent<T1>(node);
            auto comp2 = GetComponent<T2>(node);
            if (comp1 && comp2) {
                callback(*comp1, *comp2);
            }
        }
    }

    /// 筛选所有拥有 T 组件且满足谓词的实体，返回 Entity 列表
    template<typename T, typename Pred>
    std::vector<Entity> Filter(Pred&& predicate) {
        std::vector<Entity> result;
        for (const auto& node : m_nodes) {
            auto comp = GetComponent<T>(node);
            if (comp && predicate(*comp)) {
                result.push_back(node.handle);
            }
        }
        return result;
    }

    // ── 序列化 ──
    bool Deserialize(const std::string& path);
    bool DeserializeFromMemory(const std::string& jsonData);
    bool Serialize(const std::string& path) const;

private:
    std::string m_Name = "Untitled";
    bool m_IsDirty = false;
    std::vector<Node> m_nodes;

    // 层级与组件
    std::vector<SceneNodeData> m_nodeData;
    std::vector<std::string> m_nodeNames;
    std::unordered_map<uint32_t, std::vector<std::shared_ptr<Component>>> m_nodeComponents;
};

} // namespace Prisma

// 在 ECS 命名空间中提供 Scene 别名，方便从 ECS::World 迁移到 Scene
namespace Prisma::Core::ECS {
    using Scene = ::Prisma::Scene;
}
