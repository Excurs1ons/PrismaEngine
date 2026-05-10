#include "ScriptSystem.h"
#include "logger/Logger.h"
#include "app/Engine.h"
#include <algorithm>
#include <filesystem>

namespace Prisma {
namespace Scripting {

void ScriptSystem::Initialize() {
    LOG_INFO("ScriptSystem", "初始化脚本系统");

    // 初始化Mono运行时
    if (!Engine::Get().GetMonoRuntime().Initialize()) {
        LOG_ERROR("ScriptSystem", "无法初始化Mono运行时");
        return;
    }

    // 加载核心程序集
    LoadAssembly("scripts/PrismaEngine.Core.dll");

    m_initialized = true;
    LOG_INFO("ScriptSystem", "脚本系统初始化完成");
}

void ScriptSystem::Update(Prisma::Timestep ts) {
    if (!m_initialized)
        return;

    auto* pool = Engine::Get().GetWorld().GetComponentManager().GetPool<ScriptComponent>();
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
        Engine::Get().GetMonoRuntime().CollectGarbage();
        gcTimer = 0.0f;
    }
}

void ScriptSystem::Shutdown() {
    LOG_DEBUG("ScriptSystem", "关闭脚本系统");
    Engine::Get().GetMonoRuntime().Shutdown();
    m_initialized = false;
}

bool ScriptSystem::LoadAssembly(const std::string& assemblyPath) {
    auto& runtime = Engine::Get().GetMonoRuntime();
    if (runtime.LoadAssembly("assembly", assemblyPath)) {
        m_loadedAssemblies.push_back(assemblyPath);
        LOG_DEBUG("ScriptSystem", "成功加载程序集: {0}", assemblyPath);
        return true;
    }
    LOG_ERROR("ScriptSystem", "加载程序集失败: {0}", assemblyPath);
    return false;
}

void ScriptSystem::AddScript(Prisma::Core::ECS::EntityID entity, const std::string& scriptPath) {
    if (!m_initialized)
        return;

    auto& world      = Engine::Get().GetWorld();
    auto* scriptComp = world.GetComponent<ScriptComponent>(entity);
    if (!scriptComp) {
        scriptComp = world.AddComponent<ScriptComponent>(entity);
    }

    // 检查是否已存在
    for (const auto& path : scriptComp->scriptPaths) {
        if (path == scriptPath)
            return;
    }

    // 创建新脚本实例
    auto managedScript = Engine::Get().GetMonoRuntime().CreateScript(scriptPath);
    if (managedScript.IsValid()) {
        scriptComp->scriptPaths.push_back(scriptPath);
        scriptComp->scriptInstances.push_back(std::make_shared<ManagedObject>(std::move(managedScript)));
        scriptComp->initialized = false;
        LOG_DEBUG("ScriptSystem", "为实体 {0} 添加脚本: {1}", entity, scriptPath);
    }
}

void ScriptSystem::RemoveScript(Prisma::Core::ECS::EntityID entity, const std::string& scriptPath) {
    auto* scriptComp = Engine::Get().GetWorld().GetComponent<ScriptComponent>(entity);
    if (!scriptComp)
        return;

    auto it = std::find(scriptComp->scriptPaths.begin(), scriptComp->scriptPaths.end(), scriptPath);
    if (it != scriptComp->scriptPaths.end()) {
        const auto index = static_cast<size_t>(std::distance(scriptComp->scriptPaths.begin(), it));
        scriptComp->scriptPaths.erase(it);
        if (index < scriptComp->scriptInstances.size()) {
            scriptComp->scriptInstances.erase(scriptComp->scriptInstances.begin() + static_cast<std::ptrdiff_t>(index));
        }
        scriptComp->initialized = false;
    }
}

void ScriptSystem::ReloadScripts() {
    LOG_DEBUG("ScriptSystem", "重新加载所有脚本");

    auto* pool = Engine::Get().GetWorld().GetComponentManager().GetPool<ScriptComponent>();
    if (!pool) {
        return;
    }

    auto& components = pool->GetData();
    for (auto& script : components) {
        script.scriptInstances.clear();
        for (const auto& scriptPath : script.scriptPaths) {
            auto managedScript = Engine::Get().GetMonoRuntime().CreateScript(scriptPath);
            if (managedScript.IsValid()) {
                script.scriptInstances.push_back(std::make_shared<ManagedObject>(std::move(managedScript)));
            } else {
                LOG_WARNING("ScriptSystem", "重新创建脚本实例失败: {0}", scriptPath);
            }
        }
        script.initialized = false;
    }
}

bool ScriptSystem::CompileScripts(const std::string& projectPath) {
    namespace fs = std::filesystem;

    const fs::path root(projectPath);
    if (!fs::exists(root)) {
        LOG_ERROR("ScriptSystem", "脚本项目路径不存在: {0}", projectPath);
        return false;
    }

    bool foundCompilableInput = false;
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const auto extension = entry.path().extension().string();
        if (extension == ".dll") {
            foundCompilableInput = LoadAssembly(entry.path().string()) || foundCompilableInput;
            continue;
        }

        if (extension == ".cs" || extension == ".csproj" || extension == ".sln") {
            foundCompilableInput = true;
        }
    }

    if (!foundCompilableInput) {
        LOG_WARNING("ScriptSystem", "脚本项目目录下没有可用的程序集或源码: {0}", projectPath);
        return false;
    }

    LOG_DEBUG("ScriptSystem", "脚本项目扫描完成: {0}", projectPath);
    return true;
}

void ScriptSystem::ProcessScriptAwake(ScriptComponent& script) {
    for (auto& instance : script.scriptInstances) {
        if (instance && instance->IsValid()) {
            instance->InvokeMethod("OnAwake");
        }
    }
}

void ScriptSystem::ProcessScriptStart(ScriptComponent& script) {
    for (auto& instance : script.scriptInstances) {
        if (instance && instance->IsValid()) {
            instance->InvokeMethod("OnStart");
        }
    }
}

void ScriptSystem::ProcessScriptUpdate(ScriptComponent& script, Prisma::Timestep ts) {
    for (auto& instance : script.scriptInstances) {
        if (instance && instance->IsValid()) {
            // 传参 DeltaTime
            float dt = ts.GetSeconds();
            instance->InvokeMethod("OnUpdate", &dt);
        }
    }
}

}  // namespace Scripting
}  // namespace Prisma
