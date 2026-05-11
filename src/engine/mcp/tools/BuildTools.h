#pragma once
#include "../MCPTool.h"

namespace Prisma { class Engine; }

namespace Prisma {
namespace MCP {

class BuildInfoTool : public MCPTool {
public:
    explicit BuildInfoTool(Engine* engine);
    std::string_view GetName() const override { return "build_info"; }
    std::string_view GetDescription() const override {
        return "Get build configuration: platform, compiler, config, Vulkan version, MCP version.";
    }
    std::string_view GetCategory() const override { return "build"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

class ShaderCompileTool : public MCPTool {
public:
    explicit ShaderCompileTool(Engine* engine);
    std::string_view GetName() const override { return "shader_compile"; }
    std::string_view GetDescription() const override {
        return "Compile a shader from source code or file. Requires ShaderFactory integration.";
    }
    std::string_view GetCategory() const override { return "build"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

} // namespace MCP
} // namespace Prisma