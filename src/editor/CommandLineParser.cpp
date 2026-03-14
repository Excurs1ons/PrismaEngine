#include "CommandLineParser.h"
#include "../engine/Logger.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace Prisma {

// 版本信息
static const char* ENGINE_VERSION = "0.1.0-alpha";
static const char* BUILD_DATE = __DATE__;
static const char* BUILD_TIME = __TIME__;
static const char* GIT_COMMIT = "unknown";

CommandLineParser::CommandLineParser(int argc, char* argv[])
    : m_argc(argc), m_argv(argv) {
}

bool CommandLineParser::Parse() {
    if (m_argc < 2) {
        return true; // 默认 GUI 模式
    }

    std::vector<std::string> args;
    for (int i = 1; i < m_argc; ++i) {
        args.push_back(m_argv[i]);
    }

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        // 帮助和版本
        if (arg == "--help" || arg == "-h") {
            m_args.options["help"] = "true";
            return true;
        }
        if (arg == "--version" || arg == "-v") {
            m_args.options["version"] = "true";
            return true;
        }

        // 运行模式
        if (arg == "--mode") {
            std::string mode = GetNextValue(i);
            if (mode == "cli") {
                m_args.mode = EditorRunMode::CLI;
            } else if (mode == "batch") {
                m_args.mode = EditorRunMode::Batch;
            } else if (mode == "server") {
                m_args.mode = EditorRunMode::Server;
            } else if (mode == "gui") {
                m_args.mode = EditorRunMode::GUI;
            } else {
                LOG_ERROR("CommandLine", "未知的运行模式: {}", mode);
                return false;
            }
        }
        // 兼容旧写法
        else if (arg == "--cli") {
            m_args.mode = EditorRunMode::CLI;
        }
        else if (arg == "--batch") {
            m_args.mode = EditorRunMode::Batch;
        }
        else if (arg == "--server") {
            m_args.mode = EditorRunMode::Server;
        }
        // 命令
        else if (arg == "--command" || arg == "-c") {
            m_args.command = GetNextValue(i);
            // 收集命令参数
            while (i + 1 < args.size() && !args[i + 1].starts_with("--")) {
                m_args.commandArgs.push_back(args[++i]);
            }
        }
        // 项目路径
        else if (arg == "--project" || arg == "-p") {
            m_args.projectPath = GetNextValue(i);
        }
        // 其他选项
        else if (!ParseOption(arg)) {
            LOG_WARNING("CommandLine", "未知参数: {}", arg);
        }
    }

    return true;
}

bool CommandLineParser::ParseOption(const std::string& arg) {
    // 检查是否是选项（以 -- 开头）
    if (!arg.starts_with("--")) {
        return false;
    }

    std::string optionName = arg.substr(2);
    std::string optionValue;

    // 检查是否包含等号
    size_t equalPos = optionName.find('=');
    if (equalPos != std::string::npos) {
        optionValue = optionName.substr(equalPos + 1);
        optionName = optionName.substr(0, equalPos);
    }

    // 布尔选项
    if (optionValue.empty()) {
        m_args.options[optionName] = "true";
    } else {
        m_args.options[optionName] = optionValue;
    }

    // 处理常见选项
    if (optionName == "verbose" || optionName == "v") {
        m_args.verbose = true;
        m_args.logLevel = 1; // Debug
    }
    else if (optionName == "quiet" || optionName == "q") {
        m_args.quiet = true;
        m_args.logLevel = 4; // Error
    }
    else if (optionName == "log-level") {
        if (!optionValue.empty()) {
            m_args.logLevel = std::stoi(optionValue);
        }
    }
    else if (optionName == "log-file") {
        m_args.logFile = optionValue;
    }
    else if (optionName == "output" || optionName == "o") {
        m_args.outputPath = optionValue;
    }
    else if (optionName == "port") {
        if (!optionValue.empty()) {
            m_args.serverPort = std::stoi(optionValue);
        }
    }
    else if (optionName == "script") {
        m_args.scriptFile = optionValue;
    }
    else if (optionName == "continue-on-error") {
        m_args.continueOnError = true;
    }

    return true;
}

std::string CommandLineParser::GetNextValue(size_t& index) {
    // 首先检查当前参数是否包含等号
    std::string currentArg = m_argv[index + 1];
    size_t equalPos = currentArg.find('=');
    if (equalPos != std::string::npos) {
        return currentArg.substr(equalPos + 1);
    }

    // 否则返回下一个参数
    if (index + 1 < static_cast<size_t>(m_argc)) {
        return m_argv[++index];
    }
    return "";
}

void CommandLineParser::RegisterCommand(const std::string& name,
                                         const std::string& description,
                                         CommandLineHandler handler) {
    CommandInfo info;
    info.name = name;
    info.description = description;
    info.handler = handler;
    m_commands.push_back(info);
}

int CommandLineParser::ExecuteCommand() {
    if (m_args.command.empty()) {
        LOG_ERROR("CommandLine", "未指定命令");
        return 1;
    }

    // 查找命令
    for (const auto& cmd : m_commands) {
        if (cmd.name == m_args.command) {
            if (cmd.handler) {
                return cmd.handler(m_args.commandArgs);
            }
        }
    }

    LOG_ERROR("CommandLine", "未知命令: {}", m_args.command);
    return 1;
}

void CommandLineParser::ShowHelp() const {
    std::cout << "PrismaEngine Editor - 命令行帮助\n";
    std::cout << "==========================================\n\n";
    std::cout << "用法: Editor [选项] [命令] [参数...]\n\n";
    std::cout << "运行模式:\n";
    std::cout << "  --mode <mode>       设置运行模式\n";
    std::cout << "                      gui   - 图形界面模式（默认）\n";
    std::cout << "                      cli   - 命令行模式（无窗口）\n";
    std::cout << "                      batch - 批处理模式\n";
    std::cout << "                      server- 服务器模式\n";
    std::cout << "  --cli               命令行模式（快捷方式）\n";
    std::cout << "  --batch             批处理模式（快捷方式）\n";
    std::cout << "  --server            服务器模式（快捷方式）\n\n";
    std::cout << "命令:\n";
    std::cout << "  --command <name>    要执行的命令\n";
    std::cout << "  -c <name>           命令快捷方式\n\n";
    std::cout << "项目选项:\n";
    std::cout << "  --project <path>    指定项目路径\n";
    std::cout << "  -p <path>           项目路径快捷方式\n\n";
    std::cout << "常用选项:\n";
    std::cout << "  --verbose, -v       详细输出\n";
    std::cout << "  --quiet, -q         安静模式\n";
    std::cout << "  --log-level <n>     日志级别 (0-5)\n";
    std::cout << "  --log-file <path>   输出日志到文件\n";
    std::cout << "  --output <path>     输出目录\n";
    std::cout << "  -o <path>           输出目录快捷方式\n\n";
    std::cout << "批处理模式选项:\n";
    std::cout << "  --script <path>     执行脚本文件\n";
    std::cout << "  --continue-on-error 遇到错误继续执行\n\n";
    std::cout << "服务器模式选项:\n";
    std::cout << "  --port <n>          服务器端口（默认 8080）\n\n";
    std::cout << "信息:\n";
    std::cout << "  --help, -h          显示此帮助信息\n";
    std::cout << "  --version, -v       显示版本信息\n\n";

    if (!m_commands.empty()) {
        std::cout << "可用命令:\n";
        for (const auto& cmd : m_commands) {
            std::cout << "  " << cmd.name;
            if (!cmd.description.empty()) {
                std::cout << " - " << cmd.description;
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "示例:\n";
    std::cout << "  Editor --mode cli --command build --project /path/to/project\n";
    std::cout << "  Editor --cli -c build -p /path/to/project --verbose\n";
    std::cout << "  Editor --batch --script build_assets.lua\n";
    std::cout << "  Editor --server --port 9000\n\n";
}

void CommandLineParser::ShowVersion() const {
    std::cout << "PrismaEngine Editor\n";
    std::cout << "版本: " << GetVersion() << "\n";
    std::cout << "构建: " << GetBuildInfo() << "\n";
}

bool CommandLineParser::ShouldShowInfo() const {
    return m_args.options.find("help") != m_args.options.end() ||
           m_args.options.find("version") != m_args.options.end();
}

const char* CommandLineParser::GetVersion() {
    return ENGINE_VERSION;
}

const char* CommandLineParser::GetBuildInfo() {
    static std::string buildInfo;
    if (buildInfo.empty()) {
        std::ostringstream oss;
        oss << BUILD_DATE << " " << BUILD_TIME;
        if (std::string(GIT_COMMIT) != "unknown") {
            oss << " (commit: " << GIT_COMMIT << ")";
        }
        buildInfo = oss.str();
    }
    return buildInfo.c_str();
}

} // namespace Prisma
