#pragma once
#include "../MCPTool.h"

namespace Prisma { class Engine; }

namespace Prisma {
namespace MCP {

class RenderScreenshotTool : public MCPTool {
public:
    explicit RenderScreenshotTool(Engine* engine);
    std::string_view GetName() const override { return "render_screenshot"; }
    std::string_view GetDescription() const override {
        return "Capture a screenshot to a file. Returns the saved file path.";
    }
    std::string_view GetCategory() const override { return "render"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

class RenderDocCaptureTool : public MCPTool {
public:
    explicit RenderDocCaptureTool(Engine* engine);
    std::string_view GetName() const override { return "renderdoc_capture"; }
    std::string_view GetDescription() const override {
        return "Trigger a RenderDoc frame capture. Returns capture status.";
    }
    std::string_view GetCategory() const override { return "render"; }
    nlohmann::json GetInputSchema() const override { return {{"type", "object"}, {"properties", {}}}; }
    nlohmann::json Execute(const nlohmann::json& args) override;
private:
    Engine* m_Engine;
};

} // namespace MCP
} // namespace Prisma
