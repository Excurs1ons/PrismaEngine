#include "RenderTools.h"
#include "../../app/Engine.h"
#include "../../graphic/RenderSystem.h"
#include "../../graphic/adapters/vulkan/VulkanSwapChain.h"
#include "../../graphic/interfaces/IRenderDevice.h"
#include "../../graphic/interfaces/ISwapChain.h"

namespace Prisma {
namespace MCP {

RenderScreenshotTool::RenderScreenshotTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json RenderScreenshotTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"filename", {{"type", "string"}, {"description", "Output filename for screenshot (default: screenshot.png)"}}}
        }}
    };
}

nlohmann::json RenderScreenshotTool::Execute(const nlohmann::json& args) {
#if defined(PRISMA_ENABLE_RENDER_VULKAN)
    auto* renderSystem = m_Engine->GetRenderSystem();
    if (!renderSystem) {
        return {{"error", "Render system not available"}, {"success", false}};
    }

    auto* device = renderSystem->GetDevice();
    if (!device) {
        return {{"error", "Render device not available"}, {"success", false}};
    }

    auto* swapChain = device->GetSwapChain();
    if (!swapChain) {
        return {{"error", "Swap chain not available"}, {"success", false}};
    }

    std::string filename = "screenshot.png";
    if (args.contains("filename") && args["filename"].is_string()) {
        filename = args["filename"];
    }

    bool result = swapChain->Screenshot(filename);
    if (result) {
        return {{"success", true}, {"path", filename}};
    } else {
        return {{"error", "Screenshot capture failed"}, {"success", false}};
    }
#else
    return {{"error", "Vulkan renderer not enabled"}, {"success", false}};
#endif
}

RenderDocCaptureTool::RenderDocCaptureTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json RenderDocCaptureTool::Execute(const nlohmann::json&) {
    return {
        {"message", "RenderDoc capture not implemented"},
        {"success", false},
        {"note", "RenderDoc integration requires linking renderdoc.dll and calling TriggerCapture()"}
    };
}

} // namespace MCP
} // namespace Prisma
