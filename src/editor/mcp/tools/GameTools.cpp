#include "GameTools.h"
#include "app/Engine.h"

namespace Prisma {
namespace MCP {

GameGetStateTool::GameGetStateTool() = default;

glz::json_t GameGetStateTool::Execute(const glz::json_t& /*args*/) {
    auto& engine = Engine::Get();
    bool running = engine.IsRunning();
    std::string state = running ? "running" : "stopped";
    return {{"state", state}, {"running", running}};
}

GameSimulateTool::GameSimulateTool() = default;

glz::json_t GameSimulateTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"frames", {{"type", "integer"}, {"description", "Number of frames to simulate"}}}
        }},
        {"required", {"frames"}}
    };
}

glz::json_t GameSimulateTool::Execute(const glz::json_t& args) {
    int64_t frames = 1;
    if (args.get_object().contains("frames")) {
        frames = args["frames"].get_int64();
    }

    auto& engine = Engine::Get();
    const float deltaTime = 1.0f / 60.0f;

    for (int64_t i = 0; i < frames; ++i) {
        engine.Step(deltaTime);
    }

    return {{"frames_simulated", static_cast<double>(frames)}, {"delta_time", static_cast<double>(deltaTime)}, {"success", true}};
}

} // namespace MCP
} // namespace Prisma
