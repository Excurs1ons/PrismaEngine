#include "ScriptSystem.h"
#include "Logger.h"
#include <algorithm>

namespace Engine {
namespace Scripting {

void ScriptSystem::Initialize() {
    LOG_INFO("ScriptSystem", "初始化脚本系统");

    // 初始化Mono运行时
    if (!MonoRuntime::GetInstance().Initialize()) {
        LOG_ERROR("ScriptSystem", "无法初始化Mono运行时");
        return;
    }

    // 添加脚本搜索路径
    m_scriptPaths.push_back("scripts/");
    m_scriptPaths.push_back("Assets/Scripts/");

    // 加载核心程序集
    LoadAssembly("scripts/PrismaEngine.Core.dll");

    m_initialized = true;
    LOG_INFO("ScriptSystem", "脚本系统初始化完成");
}

void ScriptSystem::Update(float deltaTime) {
    if (!m_initialized) {
        return;
    }

    // 清理已销毁的实体
    CleanupDestroyedEntities();

    // 处理脚本生命周期
    ProcessScriptAwake();
    ProcessScriptStart();
    ProcessScriptUpdate(deltaTime);

    // 定期执行垃圾回收
    static float gcTimer = 0.0f;
    gcTimer += deltaTime;
    if (gcTimer > 5.0f) {
        MonoRuntime::GetInstance().CollectGarbage();
        gcTimer = 0.0f;
    }
}

void ScriptSystem::Shutdown() {
    LOG_INFO("ScriptSystem", "关闭脚本系统");

    // 处理销毁
    ProcessScriptDestroy();

    // 清理所有脚本
    m_entityScripts.clear();
    m_entityIndex.clear();

    // 关闭Mono运行时
    MonoRuntime::GetInstance().Shutdown();

    m_initialized = false;
}

bool ScriptSystem::LoadAssembly(const std::string& assemblyPath) {
    auto& runtime = MonoRuntime::GetInstance();

    if (runtime.LoadAssembly(assemblyPath)) {
        m_loadedAssemblies.push_back(assemblyPath);
        LOG_INFO("ScriptSystem", "成功加载程序集: {0}", assemblyPath);
        return true;
    }

    LOG_ERROR("ScriptSystem", "加载程序集失败: {0}", assemblyPath);
    return false;
}

void ScriptSystem::AddScript(ECS::EntityID entity, const std::string& scriptPath) {
    if (!m_initialized) {
        LOG_ERROR("ScriptSystem", "系统未初始化");
        return;
    }

    EntityScripts* entityScripts = GetEntityScripts(entity);
    if (!entityScripts) {
        // 为新实体创建脚本列表
        m_entityScripts.push_back({entity, {}});
        m_entityIndex[entity] = m_entityScripts.size() - 1;
        entityScripts = &m_entityScripts.back();
    }

    // 检查是否已存在相同的脚本
    for (const auto& script : entityScripts->scripts) {
        if (script->GetScriptPath() == scriptPath) {
            LOG_WARNING("ScriptSystem", "实体 {0} 已有脚本: {1}", entity, scriptPath);
            return;
        }
    }

    // 创建新脚本
    auto script = MonoRuntime::GetInstance().CreateScript(scriptPath);
    if (script) {
        // 设置游戏对象引用
        script->SetGameObject(reinterpret_cast<void*>(entity));

        entityScripts->scripts.push_back(script);
        LOG_INFO("ScriptSystem", "为实体 {0} 添加脚本: {1}", entity, scriptPath);
    } else {
        LOG_ERROR("ScriptSystem", "创建脚本失败: {0}", scriptPath);
    }
}

void ScriptSystem::RemoveScript(ECS::EntityID entity, const std::string& scriptPath) {
    EntityScripts* entityScripts = GetEntityScripts(entity);
    if (!entityScripts) {
        return;
    }

    auto it = std::find_if(entityScripts->scripts.begin(), entityScripts->scripts.end(),
                          [&scriptPath](const std::shared_ptr<ScriptComponent>& script) {
                              return script->GetScriptPath() == scriptPath;
                          });

    if (it != entityScripts->scripts.end()) {
        (*it)->Destroy();
        entityScripts->scripts.erase(it);
        LOG_INFO("ScriptSystem", "从实体 {0} 移除脚本: {1}", entity, scriptPath);
    }
}

void ScriptSystem::ClearScripts(ECS::EntityID entity) {
    EntityScripts* entityScripts = GetEntityScripts(entity);
    if (!entityScripts) {
        return;
    }

    // 销毁所有脚本
    for (auto& script : entityScripts->scripts) {
        script->Destroy();
    }
    entityScripts->scripts.clear();

    LOG_INFO("ScriptSystem", "清除实体 {0} 的所有脚本", entity);
}

void ScriptSystem::ReloadScripts() {
    LOG_INFO("ScriptSystem", "重新加载所有脚本");

    // 保存当前状态
    std::vector<EntityScripts> backup = m_entityScripts;

    // 清理
    m_entityScripts.clear();
    m_entityIndex.clear();

    // 重新创建脚本
    for (const auto& entityScripts : backup) {
        for (const auto& script : entityScripts.scripts) {
            AddScript(entityScripts.entity, script->GetScriptPath());
        }
    }
}

bool ScriptSystem::CompileScripts(const std::string& projectPath) {
    // TODO: 实现C#脚本编译
    // 可以使用csc.exe或Roslyn编译器
    LOG_WARNING("ScriptSystem", "脚本编译功能尚未实现: {0}", projectPath);
    return false;
}

const std::vector<std::shared_ptr<ScriptComponent>>& ScriptSystem::GetActiveScripts() const {
    static std::vector<std::shared_ptr<ScriptComponent>> empty;
    return empty; // TODO: 实现获取活动脚本列表
}

ScriptSystem::EntityScripts* ScriptSystem::GetEntityScripts(ECS::EntityID entity) {
    auto it = m_entityIndex.find(entity);
    if (it != m_entityIndex.end()) {
        return &m_entityScripts[it->second];
    }
    return nullptr;
}

void ScriptSystem::CleanupDestroyedEntities() {
    auto& world = ECS::World::GetInstance();

    // 检查实体是否仍然有效
    for (auto it = m_entityScripts.begin(); it != m_entityScripts.end();) {
        if (!world.IsEntityValid(it->entity)) {
            // 实体已被销毁，清理脚本
            for (auto& script : it->scripts) {
                script->Destroy();
            }

            // 更新索引
            m_entityIndex.erase(it->entity);

            // 移除
            it = m_entityScripts.erase(it);
        } else {
            ++it;
        }
    }
}

void ScriptSystem::ProcessScriptAwake() {
    for (auto& entityScripts : m_entityScripts) {
        for (auto& script : entityScripts.scripts) {
            if (script && !script->IsInitialized()) {
                script->Initialize();
            }
        }
    }
}

void ScriptSystem::ProcessScriptStart() {
    for (auto& entityScripts : m_entityScripts) {
        for (auto& script : entityScripts.scripts) {
            if (script && script->IsInitialized()) {
                // TODO: 调用Start方法
            }
        }
    }
}

void ScriptSystem::ProcessScriptUpdate(float deltaTime) {
    for (auto& entityScripts : m_entityScripts) {
        for (auto& script : entityScripts.scripts) {
            if (script && script->IsInitialized()) {
                script->Update(deltaTime);
            }
        }
    }
}

void ScriptSystem::ProcessScriptDestroy() {
    for (auto& entityScripts : m_entityScripts) {
        for (auto& script : entityScripts.scripts) {
            if (script) {
                script->Destroy();
            }
        }
    }
}

} // namespace Scripting
} // namespace Engine