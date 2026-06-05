#include "pch.h"
#include "Scene.h"
#include "Logger.h"
#include "core/EntityManager.h"
#include "core/ComponentRegistry.h"
#include "transform/Transform.h"
#include "graphic/LightComponent.h"
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <array>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <algorithm>

// ═══════════════════════════════════════════════════════════
// 场景文件格式数据结构（JSON 序列化用）
// ═══════════════════════════════════════════════════════════

namespace Prisma {

struct SceneComponentEntry {
    std::string type;
    glz::generic data;
};

struct SceneNodeFileData {
    std::string name;
    std::optional<std::string> parent;
    std::optional<std::array<float, 3>> position;  // 3D 位置
    std::optional<std::array<float, 4>> rotation;  // 四元数 x,y,z,w
    std::optional<std::array<float, 3>> scale;     // 3D 缩放
    std::vector<SceneComponentEntry> components;
};

struct SceneFileData {
    std::string name;
    std::vector<SceneNodeFileData> nodes;
};

} // namespace Prisma

// Glaze 元数据（全局命名空间）

template <>
struct glz::meta<Prisma::SceneComponentEntry> {
    static constexpr auto value = glz::object(
        "type", &Prisma::SceneComponentEntry::type,
        "data", &Prisma::SceneComponentEntry::data
    );
};

template <>
struct glz::meta<Prisma::SceneNodeFileData> {
    static constexpr auto value = glz::object(
        "name",       &Prisma::SceneNodeFileData::name,
        "parent",     &Prisma::SceneNodeFileData::parent,
        "position",   &Prisma::SceneNodeFileData::position,
        "rotation",   &Prisma::SceneNodeFileData::rotation,
        "scale",      &Prisma::SceneNodeFileData::scale,
        "components", &Prisma::SceneNodeFileData::components
    );
};

template <>
struct glz::meta<Prisma::SceneFileData> {
    static constexpr auto value = glz::object(
        "name",   &Prisma::SceneFileData::name,
        "nodes",  &Prisma::SceneFileData::nodes
    );
};

namespace Prisma {

// 生命周期

Scene::Scene() {}

Scene::~Scene() {
    for (auto node : m_nodes) {
        node.Destroy();
    }
}

Node Scene::CreateNode(const std::string& name) {
    Node node = EntityManager::Get().CreateNode();
    m_nodes.push_back(node);

    uint32_t idx = node.GetIndex();
    if (idx >= m_nodeData.size()) {
        m_nodeData.resize(idx + 1);
        m_nodeNames.resize(idx + 1);
    }
    m_nodeNames[idx] = name;
    m_nodeData[idx] = SceneNodeData{};  // 重置层级数据

    m_IsDirty = true;
    return node;
}

void Scene::RemoveNode(Node node) {
    if (!node.IsValid()) return;
    uint32_t idx = node.GetIndex();

    // 从父节点的 children 列表中移除
    if (idx < m_nodeData.size() && m_nodeData[idx].parent != UINT32_MAX) {
        auto& siblings = m_nodeData[m_nodeData[idx].parent].children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), idx), siblings.end());
    }

    // 递归销毁子节点
    if (idx < m_nodeData.size()) {
        auto childrenCopy = m_nodeData[idx].children;
        for (uint32_t childIdx : childrenCopy) {
            Node child;
            child.handle = childIdx | (EntityManager::Get().GetRenderData()->generation[childIdx] << 16);
            RemoveNode(child);
        }
        m_nodeData[idx] = SceneNodeData{};
    }

    // 清除组件
    m_nodeComponents.erase(node.handle);

    // 从场景节点列表中移除
    auto it = std::find(m_nodes.begin(), m_nodes.end(), node);
    if (it != m_nodes.end()) {
        m_nodes.erase(it);
    }

    node.Destroy();
    m_IsDirty = true;
}

void Scene::Update(Timestep ts) {
    // 遍历所有节点的组件并更新
    for (auto& [handle, components] : m_nodeComponents) {
        for (auto& comp : components) {
            if (comp->IsEnabled()) {
                comp->Update(ts);
            }
        }
    }
}

// Node 名称

std::string Scene::GetNodeName(Node node) const {
    uint32_t idx = node.GetIndex();
    if (idx < m_nodeNames.size()) {
        return m_nodeNames[idx];
    }
    return "";
}

void Scene::SetNodeName(Node node, const std::string& name) {
    uint32_t idx = node.GetIndex();
    if (idx < m_nodeNames.size()) {
        m_nodeNames[idx] = name;
    }
}

// 层级 API

void Scene::SetParent(Node child, Node parent) {
    if (!child.IsValid()) return;
    uint32_t childIdx = child.GetIndex();
    uint32_t parentIdx = parent.IsValid() ? parent.GetIndex() : UINT32_MAX;

    if (childIdx >= m_nodeData.size()) return;
    if (parentIdx != UINT32_MAX && parentIdx >= m_nodeData.size()) return;

    // 从旧父节点移除
    auto& oldParent = m_nodeData[childIdx].parent;
    if (oldParent != UINT32_MAX && oldParent < m_nodeData.size()) {
        auto& siblings = m_nodeData[oldParent].children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), childIdx), siblings.end());
    }

    // 设置新父节点
    m_nodeData[childIdx].parent = parentIdx;
    if (parentIdx != UINT32_MAX) {
        m_nodeData[parentIdx].children.push_back(childIdx);
    }
}

std::vector<Node> Scene::GetChildren(Node node) const {
    std::vector<Node> result;
    if (!node.IsValid()) return result;
    uint32_t idx = node.GetIndex();
    if (idx >= m_nodeData.size()) return result;

    for (uint32_t childIdx : m_nodeData[idx].children) {
        Node child;
        child.handle = childIdx | (EntityManager::Get().GetRenderData()->generation[childIdx] << 16);
        result.push_back(child);
    }
    return result;
}

Node Scene::GetParent(Node node) const {
    if (!node.IsValid()) return Node{};
    uint32_t idx = node.GetIndex();
    if (idx >= m_nodeData.size() || m_nodeData[idx].parent == UINT32_MAX) return Node{};

    uint32_t parentIdx = m_nodeData[idx].parent;
    Node parent;
    parent.handle = parentIdx | (EntityManager::Get().GetRenderData()->generation[parentIdx] << 16);
    return parent;
}

std::vector<Node> Scene::GetRootNodes() const {
    std::vector<Node> result;
    for (auto& node : m_nodes) {
        uint32_t idx = node.GetIndex();
        if (idx < m_nodeData.size() && m_nodeData[idx].parent == UINT32_MAX) {
            result.push_back(node);
        }
    }
    return result;
}

Matrix4x4 Scene::GetWorldTransform(Node node) const {
    Matrix4x4 world(1.0f);
    Node current = node;
    while (current.IsValid()) {
        auto transform = GetComponent<Transform>(current);
        if (transform) {
            world = transform->GetMatrix() * world;
        }
        current = GetParent(current);
    }
    return world;
}

// 组件 API

const std::vector<std::shared_ptr<Component>>& Scene::GetComponents(Node node) const {
    static std::vector<std::shared_ptr<Component>> empty;
    auto it = m_nodeComponents.find(node.handle);
    if (it != m_nodeComponents.end()) {
        return it->second;
    }
    return empty;
}

void Scene::RemoveComponent(Node node, Component* comp) {
    auto it = m_nodeComponents.find(node.handle);
    if (it != m_nodeComponents.end()) {
        auto& vec = it->second;
        vec.erase(std::remove_if(vec.begin(), vec.end(),
            [comp](const auto& ptr) { return ptr.get() == comp; }),
            vec.end());
        m_IsDirty = true;
    }
}

// 相机

std::shared_ptr<Prisma::Graphic::ICamera> Scene::GetMainCamera() {
    for (auto& node : m_nodes) {
        auto it = m_nodeComponents.find(node.handle);
        if (it == m_nodeComponents.end()) continue;
        for (auto& comp : it->second) {
            auto camera = std::dynamic_pointer_cast<Prisma::Graphic::ICamera>(comp);
            if (camera) return camera;
        }
    }
    return nullptr;
}

std::vector<Prisma::Graphic::Light> Scene::GetLights() const {
    std::vector<Prisma::Graphic::Light> result;
    for (auto& node : m_nodes) {
        auto it = m_nodeComponents.find(node.handle);
        if (it == m_nodeComponents.end()) continue;
        for (auto& comp : it->second) {
            auto lightComp = std::dynamic_pointer_cast<Prisma::Graphic::LightComponent>(comp);
            if (lightComp && lightComp->IsEnabled()) {
                result.push_back(lightComp->GetLightData());
            }
        }
    }
    return result;
}

// 序列化

bool Scene::Deserialize(const std::string& path) {
    auto data = Platform::ReadBinaryFile(path.c_str());
    if (data.empty()) {
        LOG_ERROR("Scene", "读取场景文件失败: {0}", path);
        return false;
    }

    std::string buffer(data.begin(), data.end());
    return DeserializeFromMemory(buffer);
}

bool Scene::DeserializeFromMemory(const std::string& jsonData) {
    SceneFileData sfd;
    std::string buffer = jsonData;

    // 跳过 UTF-8 BOM (EF BB BF)
    if (buffer.size() >= 3 &&
        (static_cast<uint8_t>(buffer[0]) == 0xEF) &&
        (static_cast<uint8_t>(buffer[1]) == 0xBB) &&
        (static_cast<uint8_t>(buffer[2]) == 0xBF)) {
        buffer.erase(0, 3);
    }

    // 解析 JSON
    auto error = glz::read<glz::opts{ .comments = true, .error_on_unknown_keys = false }>(sfd, buffer);
    if (error) {
        LOG_ERROR("Scene", "解析场景数据失败: {0}", glz::format_error(error, ""));
        return false;
    }

    SetName(sfd.name);
    
    // 清理当前场景数据
    m_nodes.clear();
    m_nodeData.clear();
    m_nodeNames.clear();
    m_nodeComponents.clear();

    // 第一遍：创建所有 Node
    std::unordered_map<std::string, Node> nameToNode;
    for (auto& nfd : sfd.nodes) {
        Node node = CreateNode(nfd.name);
        nameToNode[nfd.name] = node;
    }

    // 第二遍：设置层级关系 + 组件
    for (auto& nfd : sfd.nodes) {
        Node node = nameToNode[nfd.name];
        uint32_t idx = node.GetIndex();

        // 层级
        if (nfd.parent && !nfd.parent->empty()) {
            auto it = nameToNode.find(*nfd.parent);
            if (it != nameToNode.end()) {
                uint32_t pIdx = it->second.GetIndex();
                m_nodeData[idx].parent = pIdx;
                m_nodeData[pIdx].children.push_back(idx);
            }
        }

        // Transform 组件
        auto transform = AddComponent<Transform>(node);
        Transform::Data td;
        if (nfd.position) td.position = *nfd.position;
        if (nfd.rotation) td.rotation = *nfd.rotation;
        if (nfd.scale)    td.scale    = *nfd.scale;
        transform->SetData(td);

        // 其他组件
        auto& reg = ComponentRegistry::Get();
        for (auto& compEntry : nfd.components) {
            auto comp = reg.Create(compEntry.type);
            if (!comp) {
                LOG_WARN("Scene", "创建组件失败: {0}", compEntry.type);
                continue;
            }
            comp->SetOwnerNode(node, this);
            comp->Initialize();

            auto json = compEntry.data.dump();
            if (json) {
                reg.DeserializeComponent(*comp, compEntry.type, *json);
            }

            m_nodeComponents[node.handle].push_back(std::move(comp));
        }
    }

    LOG_DEBUG("Scene", "场景已从内存加载: {0} ({1} 个 Node)", sfd.name, m_nodes.size());
    return true;
}

bool Scene::Serialize(const std::string& path) const {
    SceneFileData sfd;
    sfd.name = m_Name;

    // 构建 index→name 映射
    std::unordered_map<uint32_t, std::string> idxToName;
    for (size_t i = 0; i < m_nodeNames.size(); i++) {
        idxToName[static_cast<uint32_t>(i)] = m_nodeNames[i];
    }

    auto& reg = ComponentRegistry::Get();

    for (auto& node : m_nodes) {
        uint32_t idx = node.GetIndex();
        SceneNodeFileData nfd;
        nfd.name = (idx < m_nodeNames.size()) ? m_nodeNames[idx] : "Node";

        // 父节点
        if (idx < m_nodeData.size() && m_nodeData[idx].parent != UINT32_MAX) {
            auto it = idxToName.find(m_nodeData[idx].parent);
            if (it != idxToName.end()) {
                nfd.parent = it->second;
            }
        }

        // Transform 组件
        auto transform = GetComponent<Transform>(node);
        if (transform) {
            auto td = transform->GetData();
            nfd.position = td.position;
            nfd.rotation = td.rotation;
            nfd.scale    = td.scale;
        }

        // 其他组件
        auto comps = GetComponents(node);
        for (auto& comp : comps) {
            auto typeName = reg.GetTypeName(*comp);
            if (typeName.empty()) continue;

            std::string tn(typeName);
            if (!reg.CanSerialize(tn)) continue;

            SceneComponentEntry entry;
            entry.type = tn;
            auto json = reg.SerializeComponent(*comp);
            if (!json.empty()) {
                auto ec = glz::read_json(entry.data, json);
                if (ec) {
                    LOG_WARN("Scene", "组件数据序列化失败: {0}", tn);
                }
            }
            nfd.components.push_back(std::move(entry));
        }

        sfd.nodes.push_back(std::move(nfd));
    }

    auto error = glz::write_file_json(sfd, path, std::string{});
    if (error) {
        LOG_ERROR("Scene", "场景序列化失败: {0}", glz::format_error(error, ""));
        return false;
    }

    LOG_INFO("Scene", "场景已保存: {0} ({1} 个 Node)", path, m_nodes.size());
    return true;
}

} // namespace Prisma
