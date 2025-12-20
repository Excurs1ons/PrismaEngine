#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <typeinfo>
#include <functional>
#include <mutex>
#include "Logger.h"

namespace Engine {
namespace Core {
namespace ECS {

// 前向声明
class World;
class Entity;
class ComponentManager;

// 实体ID
using EntityID = uint32_t;
const EntityID INVALID_ENTITY = 0;

// 组件类型ID
using ComponentTypeID = uint32_t;
const ComponentTypeID INVALID_COMPONENT_TYPE = 0;

// 系统类型ID
using SystemTypeID = uint32_t;
const SystemTypeID INVALID_SYSTEM_TYPE = 0;

// 组件基类
class IComponent {
public:
    virtual ~IComponent() = default;

    // 获取组件类型ID
    virtual ComponentTypeID GetTypeID() const = 0;

    // 是否启用
    bool enabled = true;
};

// 系统基类
class ISystem {
public:
    virtual ~ISystem() = default;

    // 获取系统类型ID
    virtual SystemTypeID GetTypeID() const = 0;

    // 初始化
    virtual void Initialize() {}

    // 更新
    virtual void Update(float deltaTime) = 0;

    // 销毁
    virtual void Shutdown() {}

    // 是否启用
    bool enabled = true;

protected:
    World* m_world = nullptr;
    friend class World;
};

// 组件管理器
class ComponentManager {
public:
    template<typename T>
    void RegisterComponent();

    ComponentTypeID GetComponentType(const std::type_info& typeInfo);

    template<typename T>
    T* AddComponent(EntityID entity);

    template<typename T>
    T* GetComponent(EntityID entity);

    template<typename T>
    bool HasComponent(EntityID entity);

    template<typename T>
    void RemoveComponent(EntityID entity);

    void RemoveAllComponents(EntityID entity);

    template<typename T>
    std::vector<EntityID> GetEntitiesWithComponent();

private:
    // 组件类型注册
    std::unordered_map<size_t, ComponentTypeID> m_componentTypes;
    ComponentTypeID m_nextComponentType = 1;

    // 组件数据
    struct ComponentData {
        std::vector<std::unique_ptr<IComponent>> components;
        std::unordered_map<EntityID, size_t> entityToIndex;
        std::unordered_map<size_t, EntityID> indexToEntity;
    };

    std::vector<ComponentData> m_componentArrays;
    std::mutex m_mutex;

public:
    // 用于清理操作
    void ClearComponents() {
        for (auto& data : m_componentArrays) {
            data.components.clear();
            data.entityToIndex.clear();
            data.indexToEntity.clear();
        }
    }
};

// 实体管理器
class EntityManager {
public:
    EntityManager();

    EntityID CreateEntity();
    void DestroyEntity(EntityID entity);
    bool IsEntityValid(EntityID entity) const;

    const std::vector<EntityID>& GetAliveEntities() const { return m_aliveEntities; }

    // 组件操作
    template<typename T>
    T* AddComponent(EntityID entity);

    template<typename T>
    T* GetComponent(EntityID entity);

    template<typename T>
    bool HasComponent(EntityID entity);

    template<typename T>
    void RemoveComponent(EntityID entity);

private:
    // 实体池管理
    std::vector<EntityID> m_aliveEntities;
    std::vector<EntityID> m_freeEntities;
    EntityID m_nextEntity = 1;

    ComponentManager* m_componentManager;

public:
    // 用于World类初始化
    void SetComponentManager(ComponentManager* manager) { m_componentManager = manager; }

    // 用于清理操作
    void ClearEntities() {
        m_aliveEntities.clear();
        m_freeEntities.clear();
        m_nextEntity = 1;
    }
};

// 世界 - 管理所有实体、组件和系统
class World {
public:
    static World& GetInstance();

    // 实体管理
    EntityID CreateEntity();
    void DestroyEntity(EntityID entity);
    bool IsEntityValid(EntityID entity) const;

    // 组件管理
    template<typename T>
    T* AddComponent(EntityID entity);

    template<typename T>
    T* GetComponent(EntityID entity);

    template<typename T>
    bool HasComponent(EntityID entity);

    template<typename T>
    void RemoveComponent(EntityID entity);

    // 系统管理
    template<typename T, typename... Args>
    T* AddSystem(Args&&... args);

    template<typename T>
    T* GetSystem();

    template<typename T>
    void RemoveSystem();

    // 更新
    void Update(float deltaTime);

    // 获取实体管理器
    EntityManager& GetEntityManager() { return m_entityManager; }

    // 获取组件管理器
    ComponentManager& GetComponentManager() { return m_componentManager; }

    // 清空世界
    void Clear();

    // 保存/加载
    bool SaveToFile(const std::string& filePath);
    bool LoadFromFile(const std::string& filePath);

private:
    World();
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    // 管理器
    EntityManager m_entityManager;
    ComponentManager m_componentManager;

    // 系统
    std::vector<std::unique_ptr<ISystem>> m_systems;
    std::unordered_map<size_t, SystemTypeID> m_systemTypes;
    SystemTypeID m_nextSystemType = 1;

    mutable std::mutex m_mutex;
};

// 实体类 - 提供便利接口
class Entity {
public:
    Entity() : m_id(INVALID_ENTITY), m_world(nullptr) {}
    Entity(EntityID id, World* world) : m_id(id), m_world(world) {}

    // 实体ID
    EntityID GetID() const { return m_id; }

    // 有效性
    bool IsValid() const {
        return m_id != INVALID_ENTITY && m_world && m_world->IsEntityValid(m_id);
    }

    // 组件操作
    template<typename T, typename... Args>
    T& AddComponent(Args&&... args) {
        T* component = m_world->AddComponent<T>(m_id);
        if (component) {
            *component = T(std::forward<Args>(args)...);
        }
        return *component;
    }

    template<typename T>
    T& GetComponent() {
        return *m_world->GetComponent<T>(m_id);
    }

    template<typename T>
    const T& GetComponent() const {
        return *m_world->GetComponent<T>(m_id);
    }

    template<typename T>
    bool HasComponent() const {
        return m_world->HasComponent<T>(m_id);
    }

    template<typename T>
    void RemoveComponent() {
        m_world->RemoveComponent<T>(m_id);
    }

    // 销毁实体
    void Destroy() {
        if (m_world) {
            m_world->DestroyEntity(m_id);
            m_id = INVALID_ENTITY;
        }
    }

    // 比较操作
    bool operator==(const Entity& other) const {
        return m_id == other.m_id && m_world == other.m_world;
    }

    bool operator!=(const Entity& other) const {
        return !(*this == other);
    }

    // 布尔转换
    explicit operator bool() const {
        return IsValid();
    }

private:
    EntityID m_id;
    World* m_world;
};

// 组件模板实现
template<typename T>
void ComponentManager::RegisterComponent() {
    std::lock_guard<std::mutex> lock(m_mutex);

    const std::type_info& typeInfo = typeid(T);
    if (m_componentTypes.find(typeInfo.hash_code()) == m_componentTypes.end()) {
        m_componentTypes[typeInfo.hash_code()] = m_nextComponentType++;
        m_componentArrays.emplace_back();
        LOG_DEBUG("ECS", "注册组件类型: {0}", typeid(T).name());
    }
}

template<typename T>
T* ComponentManager::AddComponent(EntityID entity) {
    std::lock_guard<std::mutex> lock(m_mutex);

    ComponentTypeID typeID = GetComponentType(typeid(T));
    if (typeID == INVALID_COMPONENT_TYPE) {
        RegisterComponent<T>();
        typeID = GetComponentType(typeid(T));
    }

    ComponentData& data = m_componentArrays[typeID - 1];

    // 检查实体是否已经有该组件
    auto it = data.entityToIndex.find(entity);
    if (it != data.entityToIndex.end()) {
        return static_cast<T*>(data.components[it->second].get());
    }

    // 添加新组件
    auto component = std::make_unique<T>();
    T* componentPtr = component.get();
    componentPtr->enabled = true;

    size_t index = data.components.size();
    data.components.push_back(std::move(component));
    data.entityToIndex[entity] = index;
    data.indexToEntity[index] = entity;

    return componentPtr;
}

template<typename T>
T* ComponentManager::GetComponent(EntityID entity) {
    std::lock_guard<std::mutex> lock(m_mutex);

    ComponentTypeID typeID = GetComponentType(typeid(T));
    if (typeID == INVALID_COMPONENT_TYPE) {
        return nullptr;
    }

    const ComponentData& data = m_componentArrays[typeID - 1];
    auto it = data.entityToIndex.find(entity);
    if (it == data.entityToIndex.end()) {
        return nullptr;
    }

    return static_cast<T*>(data.components[it->second].get());
}

template<typename T>
bool ComponentManager::HasComponent(EntityID entity) {
    return GetComponent<T>(entity) != nullptr;
}

template<typename T>
void ComponentManager::RemoveComponent(EntityID entity) {
    std::lock_guard<std::mutex> lock(m_mutex);

    ComponentTypeID typeID = GetComponentType(typeid(T));
    if (typeID == INVALID_COMPONENT_TYPE) {
        return;
    }

    ComponentData& data = m_componentArrays[typeID - 1];
    auto it = data.entityToIndex.find(entity);
    if (it == data.entityToIndex.end()) {
        return;
    }

    // 移除组件并更新索引
    size_t lastIndex = data.components.size() - 1;
    EntityID lastEntity = data.indexToEntity[lastIndex];

    data.components[it->second] = std::move(data.components[lastIndex]);
    data.entityToIndex[lastEntity] = it->second;
    data.indexToEntity[it->second] = lastEntity;

    data.entityToIndex.erase(entity);
    data.indexToEntity.erase(lastIndex);
    data.components.pop_back();
}

template<typename T>
std::vector<EntityID> ComponentManager::GetEntitiesWithComponent() {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<EntityID> entities;
    ComponentTypeID typeID = GetComponentType(typeid(T));
    if (typeID == INVALID_COMPONENT_TYPE) {
        return entities;
    }

    const ComponentData& data = m_componentArrays[typeID - 1];
    for (const auto& pair : data.entityToIndex) {
        const auto& component = static_cast<T*>(data.components[pair.second].get());
        if (component && component->enabled) {
            entities.push_back(pair.first);
        }
    }

    return entities;
}

// 世界模板实现
template<typename T>
T* World::AddComponent(EntityID entity) {
    return m_componentManager.AddComponent<T>(entity);
}

template<typename T>
T* World::GetComponent(EntityID entity) {
    return m_componentManager.GetComponent<T>(entity);
}

template<typename T>
bool World::HasComponent(EntityID entity) {
    return m_componentManager.HasComponent<T>(entity);
}

template<typename T>
void World::RemoveComponent(EntityID entity) {
    m_componentManager.RemoveComponent<T>(entity);
}

template<typename T, typename... Args>
T* World::AddSystem(Args&&... args) {
    std::lock_guard<std::mutex> lock(m_mutex);

    const std::type_info& typeInfo = typeid(T);
    auto it = m_systemTypes.find(typeInfo.hash_code());
    if (it != m_systemTypes.end()) {
        return dynamic_cast<T*>(m_systems[it->second - 1].get());
    }

    SystemTypeID typeID = m_nextSystemType++;
    m_systemTypes[typeInfo.hash_code()] = typeID;

    auto system = std::make_unique<T>(std::forward<Args>(args)...);
    system->m_world = this;
    system->Initialize();

    T* systemPtr = system.get();
    m_systems.push_back(std::move(system));

    LOG_DEBUG("ECS", "添加系统: {0}", typeid(T).name());
    return systemPtr;
}

template<typename T>
T* World::GetSystem() {
    std::lock_guard<std::mutex> lock(m_mutex);

    const std::type_info& typeInfo = typeid(T);
    auto it = m_systemTypes.find(typeInfo.hash_code());
    if (it == m_systemTypes.end()) {
        return nullptr;
    }

    return dynamic_cast<T*>(m_systems[it->second - 1].get());
}

template<typename T>
void World::RemoveSystem() {
    std::lock_guard<std::mutex> lock(m_mutex);

    const std::type_info& typeInfo = typeid(T);
    auto it = m_systemTypes.find(typeInfo.hash_code());
    if (it == m_systemTypes.end()) {
        return;
    }

    SystemTypeID typeID = it->second;
    m_systems[typeID - 1].reset();
    m_systems.erase(m_systems.begin() + (typeID - 1));
    m_systemTypes.erase(it);

    LOG_DEBUG("ECS", "移除系统: {0}", typeid(T).name());
}

} // namespace ECS
} // namespace Core
} // namespace Engine