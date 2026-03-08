#include "CommandLineEditor.h"
#include "../engine/Logger.h"
#include "../engine/Platform.h"
#include <iostream>
#include <filesystem>
#include <sstream>
#include <unordered_map>

namespace PrismaEngine {

namespace fs = std::filesystem;

std::shared_ptr<CommandLineEditor> CommandLineEditor::GetInstance() {
    static std::shared_ptr<CommandLineEditor> instance = std::make_shared<CommandLineEditor>();
    return instance;
}

struct CommandLineEditor::Impl {
    struct CommandInfo {
        std::string description;
        CommandHandler handler;
    };
    std::unordered_map<std::string, CommandInfo> commands;
};

CommandLineEditor::CommandLineEditor() 
    : m_impl(std::make_unique<Impl>())
{
    LOG_INFO("CommandLineEditor", "初始化命令行编辑器");
    RegisterBuiltinCommands();
}

CommandLineEditor::~CommandLineEditor() {
}

void CommandLineEditor::SetArguments(const CommandLineParser::Arguments& args) {
    m_args = args;
}

int CommandLineEditor::Initialize() {
    LOG_INFO("CommandLineEditor", "初始化命令行编辑器系统");

    LogLevel logLevel = LogLevel::Info;
    if (m_args.quiet) {
        logLevel = LogLevel::Error;
    } else if (m_args.verbose) {
        logLevel = LogLevel::Debug;
    } else {
        switch (m_args.logLevel) {
            case 0: logLevel = LogLevel::Trace; break;
            case 1: logLevel = LogLevel::Debug; break;
            case 2: logLevel = LogLevel::Info; break;
            case 3: logLevel = LogLevel::Warning; break;
            case 4: logLevel = LogLevel::Error; break;
            case 5: logLevel = LogLevel::Fatal; break;
            default: logLevel = LogLevel::Info; break;
        }
    }

    Logger::GetInstance().SetMinLevel(logLevel);
    LOG_INFO("CommandLineEditor", "命令行编辑器初始化完成");
    return 0;
}

int CommandLineEditor::Run() {
    if (m_args.command.empty()) {
        ShowHelp();
        return 0;
    }
    return ExecuteCommand();
}

void CommandLineEditor::Shutdown() {
}

void CommandLineEditor::RegisterCommand(const std::string& name,
                                        const std::string& description,
                                        CommandHandler handler) {
    Impl::CommandInfo info;
    info.description = description;
    info.handler = handler;
    m_impl->commands[name] = info;
}

void CommandLineEditor::RegisterBuiltinCommands() {
    RegisterCommand("build", "构建项目", [this](const auto& args) { return CommandBuild(args); });
    RegisterCommand("clean", "清理构建产物", [this](const auto& args) { return CommandClean(args); });
    RegisterCommand("export", "导出资源/场景", [this](const auto& args) { return CommandExport(args); });
    RegisterCommand("import", "导入资源", [this](const auto& args) { return CommandImport(args); });
    RegisterCommand("package", "打包项目", [this](const auto& args) { return CommandPackage(args); });
    RegisterCommand("info", "显示项目信息", [this](const auto& args) { return CommandShowInfo(args); });
    RegisterCommand("validate", "验证项目配置", [this](const auto& args) { return CommandValidate(args); });
    RegisterCommand("run", "运行项目", [this](const auto& args) { return CommandRun(args); });
}

int CommandLineEditor::ExecuteCommand() {
    auto it = m_impl->commands.find(m_args.command);
    if (it == m_impl->commands.end()) {
        LOG_ERROR("CommandLineEditor", "未知命令: {}", m_args.command);
        ShowHelp();
        return 1;
    }

    if (!it->second.handler) return 1;

    try {
        return it->second.handler(m_args.commandArgs);
    } catch (const std::exception& e) {
        LOG_ERROR("CommandLineEditor", "执行命令时发生错误: {}", e.what());
        return 1;
    }
}

void CommandLineEditor::ShowHelp() {
    std::cout << "\nPrismaEngine 命令行编辑器\n";
    for (const auto& [name, info] : m_impl->commands) {
        std::cout << "  " << name << " - " << info.description << "\n";
    }
}

int CommandLineEditor::CommandBuild(const std::vector<std::string>& args) {
    (void)args;
    return 0;
}

int CommandLineEditor::CommandClean(const std::vector<std::string>& args) {
    (void)args;
    return 0;
}

int CommandLineEditor::CommandExport(const std::vector<std::string>& args) {
    (void)args;
    return 0;
}

int CommandLineEditor::CommandImport(const std::vector<std::string>& args) {
    (void)args;
    return 0;
}

int CommandLineEditor::CommandPackage(const std::vector<std::string>& args) {
    (void)args;
    return 0;
}

int CommandLineEditor::CommandShowInfo(const std::vector<std::string>& args) {
    (void)args;
    return 0;
}

int CommandLineEditor::CommandValidate(const std::vector<std::string>& args) {
    (void)args;
    return 0;
}

int CommandLineEditor::CommandRun(const std::vector<std::string>& args) {
    (void)args;
    return 0;
}

} // namespace PrismaEngine
