#pragma once

#include "Timestep.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <algorithm>
#include <bitset>

namespace Prisma {
namespace Core {
namespace ECS {

using EntityID = uint32_t;
const EntityID INVALID_ENTITY = 0;

// ── 已废弃：请使用 Scene + Component 替代 ──

// 系统基类（已废弃 — 改用 Scene 直接管理组件更新）
class [[deprecated("Use Scene+Component instead")]] ISystem {
public:
    virtual ~ISystem() = default;
    virtual void Initialize() {}
    virtual void Update(Prisma::Timestep ts) = 0;
    virtual void Shutdown() {}
    bool enabled = true;

protected:
    class World* m_world = nullptr;
    friend class World;
};

// 基本 ECS 实现（已废弃 — 改用 Scene::CreateEntity / Scene::ForEach 等）
class [[deprecated("Use Scene+Component instead")]] World {
public:
    static World& Get() {
        static World instance;
        return instance;
    }

    void Update(Prisma::Timestep ts) {
        for (auto& system : m_systems) {
            if (system->enabled) {
                system->Update(ts);
            }
        }
    }

    template<typename T>
    void AddSystem() {
        auto system = std::make_shared<T>();
        system->m_world = this;
        system->Initialize();
        m_systems.push_back(system);
    }

    // Stub for component manager
    struct ComponentManager {
        template<typename T>
        struct Pool {
            std::vector<T>& GetData() { static std::vector<T> data; return data; }
        };
        template<typename T>
        Pool<T>* GetPool() { static Pool<T> pool; return &pool; }
    };

    ComponentManager& GetComponentManager() { static ComponentManager cm; return cm; }

    template<typename T>
    T* GetComponent(EntityID entity) { (void)entity; return nullptr; }

    template<typename T>
    T* AddComponent(EntityID entity) { (void)entity; return nullptr; }

private:
    std::vector<std::shared_ptr<ISystem>> m_systems;
};

} // namespace ECS
} // namespace Core
} // namespace Prisma
