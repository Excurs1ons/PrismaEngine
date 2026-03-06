#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <typeinfo>
#include <functional>
#include <mutex>
#include "Logger.h"

namespace PrismaEngine {
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

/**
 * @brief 组件类型注册表（静态）
 * 使用模板技术在编译期分配类型 ID，避免虚函数
 */
class ComponentRegistry {
public:
    template<typename T>
    static ComponentTypeID GetTypeID() {
        static const ComponentTypeID id = m_nextID++;
        return id;
    }
private:
    static inline ComponentTypeID m_nextID = 1;
};

// 系统基类
class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void Initialize() {}
    virtual void Update(float deltaTime) = 0;
    virtual void Shutdown() {}
    bool enabled = true;

protected:
    World* m_world = nullptr;
    friend class World;
};

/**
 * @brief 类型擦除的组件池接口
 */
class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void Remove(EntityID entity) = 0;
    virtual void Clear() = 0;
};

/**
 * @brief 连续内存组件池 (Data-Oriented)
 * 存储组件对象本身而非指针，保证缓存局部性
 */
template<typename T>
class ComponentPool : public IComponentPool {
public:
    T* Add(EntityID entity) {
        auto it = m_entityToIndex.find(entity);
        if (it != m_entityToIndex.end()) {
            return &m_components[it->second];
        }
        size_t index = m_components.size();
        m_entityToIndex[entity] = index;
        m_indexToEntity[index] = entity;
        m_components.emplace_back();
        return &m_components[index];
    }

    T* Get(EntityID entity) {
        auto it = m_entityToIndex.find(entity);
        return (it != m_entityToIndex.end()) ? &m_components[it->second] : nullptr;
    }

    void Remove(EntityID entity) override {
        auto it = m_entityToIndex.find(entity);
        if (it == m_entityToIndex.end()) return;

        size_t indexToRemove = it->second;
        size_t lastIndex = m_components.size() - 1;
        EntityID lastEntity = m_indexToEntity[lastIndex];

        // 将最后一个元素移动到删除位置，保持内存连续 (Swap-and-pop)
        if (indexToRemove != lastIndex) {
            m_components[indexToRemove] = std::move(m_components[lastIndex]);
            m_entityToIndex[lastEntity] = indexToRemove;
            m_indexToEntity[indexToRemove] = lastEntity;
        }

        m_entityToIndex.erase(entity);
        m_indexToEntity.erase(lastIndex);
        m_components.pop_back();
    }

    void Clear() override {
        m_components.clear();
        m_entityToIndex.clear();
        m_indexToEntity.clear();
    }

    std::vector<T>& GetData() { return m_components; }
    const std::vector<T>& GetData() const { return m_components; }

    const std::unordered_map<EntityID, size_t>& GetEntityToIndexMap() const { return m_entityToIndex; }

private:
    std::vector<T> m_components; // 连续内存存储对象本身
    std::unordered_map<EntityID, size_t> m_entityToIndex;
    std::unordered_map<size_t, EntityID> m_indexToEntity;
};

// 组件管理器
class ComponentManager {
public:
    template<typename T>
    ComponentPool<T>* GetPool() {
        std::lock_guard<std::mutex> lock(m_mutex);
        ComponentTypeID typeID = ComponentRegistry::GetTypeID<T>();
        if (typeID > m_pools.size()) {
            m_pools.resize(typeID);
        }
        if (!m_pools[typeID - 1]) {
            m_pools[typeID - 1] = std::make_unique<ComponentPool<T>>();
        }
        return static_cast<ComponentPool<T>*>(m_pools[typeID - 1].get());
    }

    template<typename T>
    T* AddComponent(EntityID entity) {
        return GetPool<T>()->Add(entity);
    }

    template<typename T>
    T* GetComponent(EntityID entity) {
        return GetPool<T>()->Get(entity);
    }

    template<typename T>
    bool HasComponent(EntityID entity) {
        return GetPool<T>()->Get(entity) != nullptr;
    }

    template<typename T>
    void RemoveComponent(EntityID entity) {
        GetPool<T>()->Remove(entity);
    }

    void RemoveAllComponents(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pool : m_pools) {
            if (pool) pool->Remove(entity);
        }
    }

private:
    std::vector<std::unique_ptr<IComponentPool>> m_pools;
    std::mutex m_mutex;
};

// 实体管理器
class EntityManager {
public:
    EntityManager() : m_componentManager(nullptr) {}

    EntityID CreateEntity() {
        EntityID id;
        if (!m_freeEntities.empty()) {
            id = m_freeEntities.back();
            m_freeEntities.pop_back();
        } else {
            id = m_nextEntity++;
        }
        m_aliveEntities.push_back(id);
        return id;
    }

    void DestroyEntity(EntityID entity) {
        auto it = std::find(m_aliveEntities.begin(), m_aliveEntities.end(), entity);
        if (it != m_aliveEntities.end()) {
            m_aliveEntities.erase(it);
            m_freeEntities.push_back(entity);
            if (m_componentManager) {
                m_componentManager->RemoveAllComponents(entity);
            }
        }
    }

    bool IsEntityValid(EntityID entity) const {
        return std::find(m_aliveEntities.begin(), m_aliveEntities.end(), entity) != m_aliveEntities.end();
    }

    const std::vector<EntityID>& GetAliveEntities() const { return m_aliveEntities; }

    void SetComponentManager(ComponentManager* manager) { m_componentManager = manager; }

    void ClearEntities() {
        m_aliveEntities.clear();
        m_freeEntities.clear();
        m_nextEntity = 1;
    }

private:
    std::vector<EntityID> m_aliveEntities;
    std::vector<EntityID> m_freeEntities;
    EntityID m_nextEntity = 1;
    ComponentManager* m_componentManager;
};

// 世界 - 管理所有实体、组件和系统
class World {
public:
    static World& GetInstance() {
        static World instance;
        return instance;
    }

    EntityID CreateEntity() {
        return m_entityManager.CreateEntity();
    }

    void DestroyEntity(EntityID entity) {
        m_entityManager.DestroyEntity(entity);
    }

    bool IsEntityValid(EntityID entity) const {
        return m_entityManager.IsEntityValid(entity);
    }

    template<typename T>
    T* AddComponent(EntityID entity) {
        return m_componentManager.AddComponent<T>(entity);
    }

    template<typename T>
    T* GetComponent(EntityID entity) {
        return m_componentManager.GetComponent<T>(entity);
    }

    template<typename T>
    bool HasComponent(EntityID entity) {
        return m_componentManager.HasComponent<T>(entity);
    }

    template<typename T>
    void RemoveComponent(EntityID entity) {
        m_componentManager.RemoveComponent<T>(entity);
    }

    template<typename T, typename... Args>
    T* AddSystem(Args&&... args) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        system->m_world = this;
        system->Initialize();
        T* ptr = system.get();
        m_systems.push_back(std::move(system));
        return ptr;
    }

    void Update(float deltaTime) {
        for (auto& system : m_systems) {
            if (system->enabled) {
                system->Update(deltaTime);
            }
        }
    }

    void Clear() {
        m_entityManager.ClearEntities();
        m_componentManager.RemoveAllComponents(0); // Simplified clear
        m_systems.clear();
    }

    EntityManager& GetEntityManager() { return m_entityManager; }
    ComponentManager& GetComponentManager() { return m_componentManager; }

private:
    World() { m_entityManager.SetComponentManager(&m_componentManager); }
    ~World() = default;

    EntityManager m_entityManager;
    ComponentManager m_componentManager;
    std::vector<std::unique_ptr<ISystem>> m_systems;
    mutable std::mutex m_mutex;
};

// 实体类 - 提供便利接口
class Entity {
public:
    Entity() : m_id(INVALID_ENTITY), m_world(nullptr) {}
    Entity(EntityID id, World* world) : m_id(id), m_world(world) {}

    EntityID GetID() const { return m_id; }
    bool IsValid() const { return m_id != INVALID_ENTITY && m_world && m_world->IsEntityValid(m_id); }

    template<typename T, typename... Args>
    T& AddComponent(Args&&... args) {
        T* component = m_world->AddComponent<T>(m_id);
        if (component) {
            *component = T(std::forward<Args>(args)...);
        }
        return *component;
    }

    template<typename T>
    T* GetComponent() { return m_world->GetComponent<T>(m_id); }

    template<typename T>
    bool HasComponent() const { return m_world->HasComponent<T>(m_id); }

    template<typename T>
    void RemoveComponent() { m_world->RemoveComponent<T>(m_id); }

    void Destroy() { if (m_world) { m_world->DestroyEntity(m_id); m_id = INVALID_ENTITY; } }

    explicit operator bool() const { return IsValid(); }

private:
    EntityID m_id;
    World* m_world;
};

} // namespace ECS
} // namespace Core
} // namespace PrismaEngine
