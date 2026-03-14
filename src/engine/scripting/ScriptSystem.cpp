#include "ScriptSystem.h"
#include "Logger.h"
#include "core/ECS.h"
#include <algorithm>

namespace Prisma {
namespace Scripting {

void ScriptSystem::Initialize() {
    LOG_INFO("ScriptSystem", "初始化脚本系统");

    // 初始化Mono运行时
    if (!MonoRuntime::Get().Initialize()) {
        LOG_ERROR("ScriptSystem", "无法初始化Mono运行时");
        return;
    }

    // 加载核心程序集
    LoadAssembly("scripts/PrismaEngine.Core.dll");

    m_initialized = true;
    LOG_INFO("ScriptSystem", "脚本系统初始化完成");
}

void ScriptSystem::Update(Timestep ts) {
    if (!m_initialized)
        return;

    auto* pool = Prisma::Core::ECS::World::Get().GetComponentManager().GetPool<ScriptComponent>();
    if (!pool)
        return;

    auto& components = pool->GetData();
    for (auto& script : components) {
        if (!script.enabled)
            continue;

        if (!script.initialized) {
            ProcessScriptAwake(script);
            ProcessScriptStart(script);
            script.initialized = true;
        }

        ProcessScriptUpdate(script, ts);
    }

    // 定期执行垃圾回收
    static float gcTimer = 0.0f;
    gcTimer += ts;
    if (gcTimer > 5.0f) {
        MonoRuntime::Get().CollectGarbage();
        gcTimer = 0.0f;
    }
}

void ScriptSystem::Shutdown() {
    LOG_INFO("ScriptSystem", "关闭脚本系统");
    MonoRuntime::Get().Shutdown();
    m_initialized = false;
}

bool ScriptSystem::LoadAssembly(const std::string& assemblyPath) {
    auto& runtime = MonoRuntime::Get();
    if (runtime.LoadAssembly("assembly", assemblyPath)) {
        m_loadedAssemblies.push_back(assemblyPath);
        LOG_INFO("ScriptSystem", "成功加载程序集: {0}", assemblyPath);
        return true;
    }
    LOG_ERROR("ScriptSystem", "加载程序集失败: {0}", assemblyPath);
    return false;
}

void ScriptSystem::AddScript(Prisma::Core::ECS::EntityID entity, const std::string& scriptPath) {
    if (!m_initialized)
        return;

    auto* world      = &Prisma::Core::ECS::World::Get();
    auto* scriptComp = world->GetComponent<ScriptComponent>(entity);
    if (!scriptComp) {
        scriptComp = world->AddComponent<ScriptComponent>(entity);
    }

    // 检查是否已存在
    for (const auto& path : scriptComp->scriptPaths) {
        if (path == scriptPath)
            return;
    }

    // 创建新脚本实例
    auto managedScript = MonoRuntime::Get().CreateScript(scriptPath);
    if (managedScript.IsValid()) {
        scriptComp->scriptPaths.push_back(scriptPath);
        LOG_INFO("ScriptSystem", "为实体 {0} 添加脚本: {1}", entity, scriptPath);
    }
}

void ScriptSystem::RemoveScript(Prisma::Core::ECS::EntityID entity, const std::string& scriptPath) {
    auto* scriptComp = Prisma::Core::ECS::World::Get().GetComponent<ScriptComponent>(entity);
    if (!scriptComp)
        return;

    auto it = std::find(scriptComp->scriptPaths.begin(), scriptComp->scriptPaths.end(), scriptPath);
    if (it != scriptComp->scriptPaths.end()) {
        (void)std::distance(scriptComp->scriptPaths.begin(), it);
        scriptComp->scriptPaths.erase(it);
    }
}

void ScriptSystem::ReloadScripts() {
    LOG_INFO("ScriptSystem", "重新加载所有脚本");
}

bool ScriptSystem::CompileScripts(const std::string& projectPath) {
    LOG_WARNING("ScriptSystem", "脚本编译功能尚未实现: {0}", projectPath);
    return false;
}

void ScriptSystem::ProcessScriptAwake(ScriptComponent& script) {
    (void)script;
}

void ScriptSystem::ProcessScriptStart(ScriptComponent& script) {
    (void)script;
}

void ScriptSystem::ProcessScriptUpdate(ScriptComponent& script, Timestep ts) {
    (void)script;
    (void)ts;
}

}  // namespace Scripting
}  // namespace Prisma
