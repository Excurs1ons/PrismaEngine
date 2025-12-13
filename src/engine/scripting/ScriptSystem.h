#pragma once

#include "core/ECS.h"
#include "MonoRuntime.h"
#include <unordered_map>
#include <vector>

namespace Engine {
namespace Scripting {

// 脚本系统 - ECS系统，管理所有脚本组件
class ScriptSystem : public Engine::Core::ECS::ISystem {
public:
    static constexpr Engine::Core::ECS::SystemTypeID TYPE_ID = 9;

      Engine::Core::ECS::SystemTypeID GetTypeID() const override { return TYPE_ID; }

    void Initialize() override;

    void Update(float deltaTime) override;

    void Shutdown() override;

    // 加载程序集
    bool LoadAssembly(const std::string& assemblyPath);

    // 为实体添加脚本
    void AddScript(Engine::Core::ECS::EntityID entity, const std::string& scriptPath);

    // 移除脚本
    void RemoveScript(Engine::Core::ECS::EntityID entity, const std::string& scriptPath);

    // 清除实体的所有脚本
    void ClearScripts(Engine::Core::ECS::EntityID entity);

    // 热重载
    void ReloadScripts();

    // 编译脚本
    bool CompileScripts(const std::string& projectPath);

    // 获取所有活动脚本
    const std::vector<std::shared_ptr<ScriptComponent>>& GetActiveScripts() const;

private:
    // 脚本管理
    struct EntityScripts {
        Engine::Core::ECS::EntityID entity;
        std::vector<std::shared_ptr<ScriptComponent>> scripts;
    };

    std::vector<EntityScripts> m_entityScripts;
    std::unordered_map<Engine::Core::ECS::EntityID, size_t> m_entityIndex;

    // 已加载的程序集
    std::vector<std::string> m_loadedAssemblies;

    // 脚本搜索路径
    std::vector<std::string> m_scriptPaths;

    // 初始化标志
    bool m_initialized = false;

    // 获取实体的脚本列表
    EntityScripts* GetEntityScripts(Engine::Core::ECS::EntityID entity);

    // 清理已销毁的实体
    void CleanupDestroyedEntities();

    // 处理脚本生命周期
    void ProcessScriptAwake();
    void ProcessScriptStart();
    void ProcessScriptUpdate(float deltaTime);
    void ProcessScriptDestroy();
};

// 脚本组件（ECS版本）
class ScriptComponent : public Engine::Core::ECS::IComponent {
public:
    static constexpr Engine::Core::ECS::ComponentTypeID TYPE_ID = Engine::Core::ECS::ComponentTypes::Script;

    Engine::Core::ECS::ComponentTypeID GetTypeID() const override { return TYPE_ID; }

    // 脚本文件路径
    std::vector<std::string> scriptPaths;

    // 脚本实例
    std::vector<std::shared_ptr<ManagedObject>> scriptInstances;

    // 是否已初始化
    bool initialized = false;
};

} // namespace Scripting
} // namespace Engine