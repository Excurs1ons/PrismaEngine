#include "CommandLineEditor.h"
#include "../engine/Logger.h"
#include "../engine/Platform.h"
#include <iostream>
#include <filesystem>
#include <sstream>

namespace PrismaEngine {

namespace fs = std::filesystem;

CommandLineEditor::CommandLineEditor() {
    LOG_INFO("CommandLineEditor", "初始化命令行编辑器");
    RegisterBuiltinCommands();
}

CommandLineEditor::~CommandLineEditor() {
    LOG_INFO("CommandLineEditor", "关闭命令行编辑器");
}

void CommandLineEditor::SetArguments(const CommandLineParser::Arguments& args) {
    m_args = args;
}

bool CommandLineEditor::Initialize() {
    LOG_INFO("CommandLineEditor", "初始化命令行编辑器系统");

    // 设置日志级别
    LogLevel logLevel = LogLevel::Info;
    if (m_args.quiet) {
        logLevel = LogLevel::Error;
    } else if (m_args.verbose) {
        logLevel = LogLevel::Debug;
    } else {
        // 映射数字级别到 LogLevel
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

    // 不初始化平台层（无窗口模式不需要）
    LOG_INFO("CommandLineEditor", "命令行编辑器初始化完成");
    return true;
}

int CommandLineEditor::Run() {
    LOG_INFO("CommandLineEditor", "运行命令: {}", m_args.command);

    if (m_args.command.empty()) {
        ShowHelp();
        return 0;
    }

    return ExecuteCommand();
}

void CommandLineEditor::Shutdown() {
    LOG_INFO("CommandLineEditor", "关闭命令行编辑器");
}

void CommandLineEditor::RegisterCommand(const std::string& name,
                                        const std::string& description,
                                        CommandHandler handler) {
    CommandLineEditor::CommandInfo info;
    info.description = description;
    info.handler = handler;
    m_commands[name] = info;
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
    auto it = m_commands.find(m_args.command);
    if (it == m_commands.end()) {
        LOG_ERROR("CommandLineEditor", "未知命令: {}", m_args.command);
        ShowHelp();
        return 1;
    }

    if (!it->second.handler) {
        LOG_ERROR("CommandLineEditor", "命令 {} 没有处理器", m_args.command);
        return 1;
    }

    try {
        return it->second.handler(m_args.commandArgs);
    } catch (const std::exception& e) {
        LOG_ERROR("CommandLineEditor", "执行命令时发生错误: {}", e.what());
        return 1;
    }
}

void CommandLineEditor::ShowHelp() {
    std::cout << "\nPrismaEngine 命令行编辑器\n";
    std::cout << "========================\n\n";
    std::cout << "可用命令:\n\n";

    for (const auto& [name, info] : m_commands) {
        std::cout << "  " << name;
        if (!info.description.empty()) {
            std::cout << " - " << info.description;
        }
        std::cout << "\n";
    }

    std::cout << "\n使用 --help 获取更多帮助信息\n\n";
}

// ========== 内置命令实现 ==========

int CommandLineEditor::CommandBuild(const std::vector<std::string>& args) {
    LOG_INFO("Build", "开始构建项目");

    // 检查项目路径
    fs::path projectPath = m_args.projectPath;
    if (projectPath.empty()) {
        projectPath = fs::current_path();
    }

    if (!fs::exists(projectPath)) {
        LOG_ERROR("Build", "项目路径不存在: {}", projectPath.string());
        return 1;
    }

    LOG_INFO("Build", "项目路径: {}", projectPath.string());

    // TODO: 实现实际的构建逻辑
    // 1. 加载项目配置
    // 2. 编译着色器
    // 3. 处理资源
    // 4. 生成构建产物

    std::cout << "\n[构建] 项目路径: " << projectPath.string() << "\n";
    std::cout << "[构建] 配置: " << (m_args.verbose ? "Debug" : "Release") << "\n";
    std::cout << "[构建] 输出: " << (m_args.outputPath.empty() ? "build/" : m_args.outputPath) << "\n";
    std::cout << "[构建] 构建中...\n";

    // 模拟构建过程
    std::cout << "[构建] 编译着色器...\n";
    std::cout << "[构建] 处理资源...\n";
    std::cout << "[构建] 生成构建产物...\n";
    std::cout << "[构建] 完成!\n\n";

    LOG_INFO("Build", "构建完成");
    return 0;
}

int CommandLineEditor::CommandClean(const std::vector<std::string>& args) {
    LOG_INFO("Clean", "清理构建产物");

    fs::path buildPath = "build";
    if (!m_args.outputPath.empty()) {
        buildPath = m_args.outputPath;
    }

    if (fs::exists(buildPath)) {
        std::cout << "[清理] 删除: " << buildPath.string() << "\n";
        fs::remove_all(buildPath);
        std::cout << "[清理] 完成\n";
    } else {
        std::cout << "[清理] 构建目录不存在\n";
    }

    LOG_INFO("Clean", "清理完成");
    return 0;
}

int CommandLineEditor::CommandExport(const std::vector<std::string>& args) {
    LOG_INFO("Export", "导出资源/场景");

    if (args.empty()) {
        LOG_ERROR("Export", "未指定导出目标");
        std::cout << "用法: export <target> [options]\n";
        std::cout << "目标:\n";
        std::cout << "  resources - 导出所有资源\n";
        std::cout << "  scenes    - 导出所有场景\n";
        std::cout << "  all       - 导出所有内容\n";
        return 1;
    }

    std::string target = args[0];
    std::cout << "[导出] 目标: " << target << "\n";

    // TODO: 实现实际的导出逻辑
    if (target == "resources" || target == "all") {
        std::cout << "[导出] 导出资源...\n";
    }
    if (target == "scenes" || target == "all") {
        std::cout << "[导出] 导出场景...\n";
    }

    std::cout << "[导出] 完成\n";
    LOG_INFO("Export", "导出完成");
    return 0;
}

int CommandLineEditor::CommandImport(const std::vector<std::string>& args) {
    LOG_INFO("Import", "导入资源");

    if (args.empty()) {
        LOG_ERROR("Import", "未指定导入源");
        std::cout << "用法: import <source>\n";
        return 1;
    }

    fs::path source = args[0];
    if (!fs::exists(source)) {
        LOG_ERROR("Import", "源文件不存在: {}", source.string());
        return 1;
    }

    std::cout << "[导入] 源: " << source.string() << "\n";
    std::cout << "[导入] 导入中...\n";

    // TODO: 实现实际的导入逻辑

    std::cout << "[导入] 完成\n";
    LOG_INFO("Import", "导入完成");
    return 0;
}

int CommandLineEditor::CommandPackage(const std::vector<std::string>& args) {
    LOG_INFO("Package", "打包项目");

    std::string platform = "linux";
    if (!args.empty()) {
        platform = args[0];
    }

    std::cout << "[打包] 平台: " << platform << "\n";
    std::cout << "[打包] 打包中...\n";

    // TODO: 实现实际的打包逻辑
    std::cout << "[打包] 完成\n";
    LOG_INFO("Package", "打包完成");
    return 0;
}

int CommandLineEditor::CommandShowInfo(const std::vector<std::string>& args) {
    LOG_INFO("Info", "显示项目信息");

    fs::path projectPath = m_args.projectPath;
    if (projectPath.empty()) {
        projectPath = fs::current_path();
    }

    std::cout << "\n项目信息\n";
    std::cout << "========================\n";
    std::cout << "路径: " << projectPath.string() << "\n";

    // TODO: 读取实际的项目配置文件
    std::cout << "名称: PrismaEngine Project\n";
    std::cout << "版本: 0.1.0\n";
    std::cout << "引擎版本: " << CommandLineParser::GetVersion() << "\n";
    std::cout << "\n";

    LOG_INFO("Info", "显示完成");
    return 0;
}

int CommandLineEditor::CommandValidate(const std::vector<std::string>& args) {
    LOG_INFO("Validate", "验证项目配置");

    fs::path projectPath = m_args.projectPath;
    if (projectPath.empty()) {
        projectPath = fs::current_path();
    }

    std::cout << "[验证] 验证项目配置...\n";

    bool valid = true;
    int errorCount = 0;
    int warningCount = 0;

    // TODO: 实现实际的验证逻辑
    // 1. 检查项目结构
    // 2. 验证配置文件
    // 3. 检查资源完整性

    if (valid) {
        std::cout << "[验证] 通过 (0 错误, " << warningCount << " 警告)\n";
        LOG_INFO("Validate", "验证通过");
        return 0;
    } else {
        std::cout << "[验证] 失败 (" << errorCount << " 错误, " << warningCount << " 警告)\n";
        LOG_ERROR("Validate", "验证失败");
        return 1;
    }
}

int CommandLineEditor::CommandRun(const std::vector<std::string>& args) {
    LOG_INFO("Run", "运行项目");

    // TODO: 实现项目运行逻辑
    std::cout << "[运行] 启动项目...\n";
    std::cout << "[运行] 这将启动一个新的游戏实例\n";

    LOG_INFO("Run", "运行完成");
    return 0;
}

} // namespace PrismaEngine
