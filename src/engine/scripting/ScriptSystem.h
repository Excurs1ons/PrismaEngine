#pragma once

#include "core/ECS.h"
#include "MonoRuntime.h"
#include <unordered_map>
#include <vector>
#include <string>

namespace PrismaEngine {
namespace Scripting {

// 脚本组件（ECS 数据组件）
struct ScriptComponent {
    // 脚本文件路径
    std::vector<std::string> scriptPaths;

    // 脚本实例
    std::vector<std::shared_ptr<ManagedObject>> scriptInstances;

    // 是否已初始化
    bool initialized = false;

    // 是否启用
    bool enabled = true;
};

// 脚本系统 - 负责处理所有实体的脚本生命周期
class ScriptSystem : public PrismaEngine::Core::ECS::ISystem {
public:
    ScriptSystem() = default;
    virtual ~ScriptSystem() = default;

    void Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;

    // 加载程序集
    bool LoadAssembly(const std::string& assemblyPath);

    // 为实体添加脚本
    void AddScript(PrismaEngine::Core::ECS::EntityID entity, const std::string& scriptPath);

    // 移除脚本
    void RemoveScript(PrismaEngine::Core::ECS::EntityID entity, const std::string& scriptPath);

    // 热重载
    void ReloadScripts();

    // 编译脚本
    bool CompileScripts(const std::string& projectPath);

private:
    // 已加载的程序集
    std::vector<std::string> m_loadedAssemblies;

    // 初始化标志
    bool m_initialized = false;

    // 处理脚本生命周期
    void ProcessScriptAwake(ScriptComponent& script);
    void ProcessScriptStart(ScriptComponent& script);
    void ProcessScriptUpdate(ScriptComponent& script, float deltaTime);
};

} // namespace Scripting
} // namespace PrismaEngine
