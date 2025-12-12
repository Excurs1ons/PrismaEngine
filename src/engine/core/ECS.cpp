#include "ECS.h"
#include "Logger.h"

namespace Engine {
namespace Core {
namespace ECS {

// ComponentManager 实现
ComponentTypeID ComponentManager::GetComponentType(const std::type_info& typeInfo) {
    auto it = m_componentTypes.find(typeInfo.hash_code());
    return (it != m_componentTypes.end()) ? it->second : INVALID_COMPONENT_TYPE;
}

void ComponentManager::RemoveAllComponents(EntityID entity) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& data : m_componentArrays) {
        auto it = data.entityToIndex.find(entity);
        if (it != data.entityToIndex.end()) {
            size_t lastIndex = data.components.size() - 1;
            EntityID lastEntity = data.indexToEntity[lastIndex];

            data.components[it->second] = std::move(data.components[lastIndex]);
            data.entityToIndex[lastEntity] = it->second;
            data.indexToEntity[it->second] = lastEntity;

            data.entityToIndex.erase(entity);
            data.indexToEntity.erase(lastIndex);
            data.components.pop_back();
        }
    }
}

// EntityManager 实现
EntityManager::EntityManager() {
    // 初始化时预留一些空间
    m_aliveEntities.reserve(1024);
    m_freeEntities.reserve(256);
}

EntityID EntityManager::CreateEntity() {
    EntityID entity;

    if (!m_freeEntities.empty()) {
        entity = m_freeEntities.back();
        m_freeEntities.pop_back();
    } else {
        entity = m_nextEntity++;
    }

    m_aliveEntities.push_back(entity);
    LOG_DEBUG("ECS", "创建实体: {0}", entity);

    return entity;
}

void EntityManager::DestroyEntity(EntityID entity) {
    auto it = std::find(m_aliveEntities.begin(), m_aliveEntities.end(), entity);
    if (it != m_aliveEntities.end()) {
        // 移除所有组件
        m_componentManager->RemoveAllComponents(entity);

        // 从活动实体列表中移除
        m_aliveEntities.erase(it);

        // 添加到空闲列表
        m_freeEntities.push_back(entity);

        LOG_DEBUG("ECS", "销毁实体: {0}", entity);
    }
}

bool EntityManager::IsEntityValid(EntityID entity) const {
    if (entity == INVALID_ENTITY) {
        return false;
    }

    return std::find(m_aliveEntities.begin(), m_aliveEntities.end(), entity) != m_aliveEntities.end();
}

// World 实现
World::World() : m_entityManager(), m_componentManager() {
    m_entityManager.m_componentManager = &m_componentManager;
    LOG_INFO("ECS", "ECS世界初始化");
}

World::~World() {
    Clear();
    LOG_INFO("ECS", "ECS世界销毁");
}

World& World::GetInstance() {
    static World instance;
    return instance;
}

EntityID World::CreateEntity() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entityManager.CreateEntity();
}

void World::DestroyEntity(EntityID entity) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entityManager.DestroyEntity(entity);
}

bool World::IsEntityValid(EntityID entity) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entityManager.IsEntityValid(entity);
}

void World::Update(float deltaTime) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 更新所有系统
    for (auto& system : m_systems) {
        if (system && system->enabled) {
            system->Update(deltaTime);
        }
    }
}

void World::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 清空所有实体
    m_entityManager.m_aliveEntities.clear();
    m_entityManager.m_freeEntities.clear();
    m_entityManager.m_nextEntity = 1;

    // 清空所有组件
    for (auto& data : m_componentManager.m_componentArrays) {
        data.components.clear();
        data.entityToIndex.clear();
        data.indexToEntity.clear();
    }

    // 清空系统
    m_systems.clear();
    m_systemTypes.clear();

    LOG_INFO("ECS", "ECS世界已清空");
}

bool World::SaveToFile(const std::string& filePath) {
    // TODO: 实现世界序列化
    LOG_WARNING("ECS", "世界保存功能尚未实现: {0}", filePath);
    return false;
}

bool World::LoadFromFile(const std::string& filePath) {
    // TODO: 实现世界反序列化
    LOG_WARNING("ECS", "世界加载功能尚未实现: {0}", filePath);
    return false;
}

} // namespace ECS
} // namespace Core
} // namespace Engine