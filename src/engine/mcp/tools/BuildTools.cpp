#include "BuildTools.h"
#include "app/Engine.h"

namespace Prisma {
namespace MCP {

// ---- BuildInfoTool ----

BuildInfoTool::BuildInfoTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json BuildInfoTool::GetInputSchema() const {
    return {{"type", "object"}, {"properties", {}}};
}

nlohmann::json BuildInfoTool::Execute(const nlohmann::json& /*args*/) {
    return {
        {"platform",
#if defined(_WIN32)
            "windows"
#elif defined(__linux__)
            "linux"
#elif defined(__ANDROID__)
            "android"
#else
            "unknown"
#endif
        },
        {"compiler",
#if defined(_MSC_VER)
            "msvc"
#elif defined(__clang__)
            "clang"
#elif defined(__GNUC__)
            "gcc"
#else
            "unknown"
#endif
        },
        {"configuration",
#if defined(_DEBUG) || defined(DEBUG)
            "debug"
#else
            "release"
#endif
        },
        {"vulkan_version", "1.3"},
        {"mcp_version", "1.0.0"}
    };
}

// ---- ShaderCompileTool ----

ShaderCompileTool::ShaderCompileTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json ShaderCompileTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"source", {{"type", "string"}, {"description", "Shader source code (GLSL for Vulkan)"}}},
            {"type", {{"type", "string"}, {"description", "Shader type: vertex, fragment, compute"}}},
            {"entry_point", {{"type", "string"}, {"description", "Entry point name (default: main)"}}}
        }}
    };
}

nlohmann::json ShaderCompileTool::Execute(const nlohmann::json& args) {
    return {
        {"error", "Shader compilation requires ShaderFactory integration. Placeholder implementation."}
    };
}

} // namespace MCP
} // namespace Prisma