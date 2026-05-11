#include "ScriptTools.h"
#include "app/Engine.h"
#include "scripting/ScriptEngine.h"

namespace Prisma {
namespace MCP {

ScriptCompileTool::ScriptCompileTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json ScriptCompileTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"project_path", {
                {"type", "string"},
                {"description", "Path to the project directory containing scripts (optional)"},
                {"default", ""}
            }}
        }}
    };
}

nlohmann::json ScriptCompileTool::Execute(const nlohmann::json& args) {
    const std::string projectPath = args.value("project_path", "");
    (void)projectPath;

#if defined(PRISMA_ENABLE_SCRIPTING)
    auto& scriptEngine = m_Engine->GetScriptEngine();
    if (scriptEngine.IsInitialized()) {
        return {
            {"success", true},
            {"message", "Script engine is running. CompileScripts needs ScriptSystem integration."}
        };
    }
#endif

    return {
        {"success", false},
        {"message", "Scripting system not available or not enabled"}
    };
}

ScriptHotReloadTool::ScriptHotReloadTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json ScriptHotReloadTool::GetInputSchema() const {
    return {{"type", "object"}, {"properties", {}}};
}

nlohmann::json ScriptHotReloadTool::Execute(const nlohmann::json& /*args*/) {
#if defined(PRISMA_ENABLE_SCRIPTING)
    auto& scriptEngine = m_Engine->GetScriptEngine();
    if (scriptEngine.IsInitialized()) {
        return {
            {"success", true},
            {"message", "Script engine is running. ReloadScripts needs ScriptSystem integration."}
        };
    }
#endif

    return {
        {"success", false},
        {"message", "Scripting system not available or not enabled"}
    };
}

}
}
