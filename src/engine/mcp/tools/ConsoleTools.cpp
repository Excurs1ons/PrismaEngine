#include "ConsoleTools.h"
#include "app/Engine.h"
#include "app/CommandLineParser.h"
#include "logger/Logger.h"

namespace Prisma {
namespace MCP {

ConsoleExecuteTool::ConsoleExecuteTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json ConsoleExecuteTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"command", {{"type", "string"}, {"description", "Console command string to execute"}}},
            {"args", {{"type", "string"}, {"description", "Optional command arguments"}}}
        }},
        {"required", {"command"}}
    };
}

nlohmann::json ConsoleExecuteTool::Execute(const nlohmann::json& args) {
    std::string command = args.value("command", "");
    if (command.empty()) {
        return {{"error", "Command cannot be empty"}};
    }

    auto& cli = CommandLineParser::Get();

    return {
        {"success", true},
        {"command", command},
        {"note", "Console command execution - integrate with runtime console system"}
    };
}

LogFilterTool::LogFilterTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json LogFilterTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"level", {{"type", "string"}, {"description", "Log level filter: trace, debug, info, warn, error"}}},
            {"category", {{"type", "string"}, {"description", "Log category filter (e.g. 'Engine', 'MCP')"}}, {"default", ""}},
            {"limit", {{"type", "integer"}, {"description", "Maximum entries to return"}, {"default", 50}}}
        }}
    };
}

nlohmann::json LogFilterTool::Execute(const nlohmann::json& args) {
    std::string levelStr = args.value("level", "info");
    std::string category = args.value("category", "");
    int limit = args.value("limit", 50);

    LogLevel level = LogLevel::Info;
    if (levelStr == "trace") {
        level = LogLevel::Trace;
    } else if (levelStr == "debug") {
        level = LogLevel::Debug;
    } else if (levelStr == "info") {
        level = LogLevel::Info;
    } else if (levelStr == "warn" || levelStr == "warning") {
        level = LogLevel::Warning;
    } else if (levelStr == "error") {
        level = LogLevel::Error;
    }

    auto& logger = Logger::Get();
    auto minLevel = logger.GetMinLevel();

    return {
        {"level", levelStr},
        {"category", category},
        {"limit", limit},
        {"entries", nlohmann::json::array()},
        {"note", "Log filtering - buffer storage pending implementation"}
    };
}

} // namespace MCP
} // namespace Prisma
