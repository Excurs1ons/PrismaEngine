#include "CommandLineEditor.h"
#include "../engine/Logger.h"
#include "../engine/Platform.h"
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <unordered_map>

namespace Prisma {

namespace fs = std::filesystem;

namespace {

fs::path ResolveProjectPath(const CommandLineParser::Arguments& arguments) {
    return arguments.projectPath.empty() ? fs::current_path() : fs::path(arguments.projectPath);
}

fs::path ResolveOutputPath(const CommandLineParser::Arguments& arguments,
                           const fs::path& projectRoot,
                           const fs::path& fallback) {
    return arguments.outputPath.empty() ? projectRoot / fallback : fs::path(arguments.outputPath);
}

std::string QuotePath(const fs::path& path) {
    return "\"" + path.string() + "\"";
}

int RunProcess(const std::string& command) {
    LOG_INFO("CommandLineEditor", "执行命令: {}", command);
    return std::system(command.c_str());
}

}  // namespace

std::shared_ptr<CommandLineEditor> CommandLineEditor::Get() {
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

    Logger::Get().SetMinLevel(logLevel);
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
    const fs::path projectRoot = ResolveProjectPath(m_args);
    const fs::path buildDir = ResolveOutputPath(m_args, projectRoot, "build");

    if (!fs::exists(projectRoot / "CMakeLists.txt")) {
        LOG_ERROR("CommandLineEditor", "项目目录缺少 CMakeLists.txt: {}", projectRoot.string());
        return 1;
    }

    fs::create_directories(buildDir);

    std::ostringstream configureCommand;
    configureCommand << "cmake -S " << QuotePath(projectRoot) << " -B " << QuotePath(buildDir);
    for (const auto& arg : args) {
        configureCommand << " " << arg;
    }

    if (RunProcess(configureCommand.str()) != 0) {
        return 1;
    }

    std::ostringstream buildCommand;
    buildCommand << "cmake --build " << QuotePath(buildDir);
    if (args.empty()) {
        buildCommand << " -j4";
    }

    return RunProcess(buildCommand.str());
}

int CommandLineEditor::CommandClean(const std::vector<std::string>& args) {
    const fs::path projectRoot = ResolveProjectPath(m_args);
    fs::path cleanTarget = ResolveOutputPath(m_args, projectRoot, "build");
    if (!args.empty()) {
        cleanTarget = fs::path(args.front());
    }

    if (!fs::exists(cleanTarget)) {
        LOG_WARNING("CommandLineEditor", "清理目标不存在: {}", cleanTarget.string());
        return 0;
    }

    std::error_code ec;
    const auto removed = fs::remove_all(cleanTarget, ec);
    if (ec) {
        LOG_ERROR("CommandLineEditor", "清理失败: {} ({})", cleanTarget.string(), ec.message());
        return 1;
    }

    LOG_INFO("CommandLineEditor", "已删除 {} 个条目: {}", removed, cleanTarget.string());
    return 0;
}

int CommandLineEditor::CommandExport(const std::vector<std::string>& args) {
    const fs::path projectRoot = ResolveProjectPath(m_args);
    const fs::path sourceDir = args.empty() ? projectRoot / "assets" : fs::path(args.front());
    const fs::path outputDir = ResolveOutputPath(m_args, projectRoot, "export");

    if (!fs::exists(sourceDir)) {
        LOG_ERROR("CommandLineEditor", "导出源目录不存在: {}", sourceDir.string());
        return 1;
    }

    fs::create_directories(outputDir);

    std::error_code ec;
    fs::copy(sourceDir, outputDir, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    if (ec) {
        LOG_ERROR("CommandLineEditor", "导出失败: {}", ec.message());
        return 1;
    }

    LOG_INFO("CommandLineEditor", "导出完成: {}", outputDir.string());
    return 0;
}

int CommandLineEditor::CommandImport(const std::vector<std::string>& args) {
    if (args.empty()) {
        LOG_ERROR("CommandLineEditor", "import 命令至少需要一个待导入路径");
        return 1;
    }

    const fs::path projectRoot = ResolveProjectPath(m_args);
    const fs::path targetDir = ResolveOutputPath(m_args, projectRoot, fs::path("assets") / "imported");
    fs::create_directories(targetDir);

    for (const auto& arg : args) {
        const fs::path sourcePath(arg);
        if (!fs::exists(sourcePath)) {
            LOG_WARNING("CommandLineEditor", "跳过不存在的路径: {}", sourcePath.string());
            continue;
        }

        std::error_code ec;
        fs::copy(sourcePath,
                 targetDir / sourcePath.filename(),
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing,
                 ec);
        if (ec) {
            LOG_ERROR("CommandLineEditor", "导入失败: {} ({})", sourcePath.string(), ec.message());
            return 1;
        }
    }

    LOG_INFO("CommandLineEditor", "导入完成: {}", targetDir.string());
    return 0;
}

int CommandLineEditor::CommandPackage(const std::vector<std::string>& args) {
    const fs::path projectRoot = ResolveProjectPath(m_args);
    const fs::path packageDir = args.empty() ? ResolveOutputPath(m_args, projectRoot, "package") : fs::path(args.front());
    const std::vector<std::pair<fs::path, fs::path>> packageItems = {
        {projectRoot / "build/bin", packageDir / "bin"},
        {projectRoot / "build/lib", packageDir / "lib"},
        {projectRoot / "assets", packageDir / "assets"}
    };

    fs::create_directories(packageDir);
    for (const auto& [sourcePath, destinationPath] : packageItems) {
        if (!fs::exists(sourcePath)) {
            LOG_WARNING("CommandLineEditor", "打包时跳过缺失路径: {}", sourcePath.string());
            continue;
        }

        std::error_code ec;
        fs::copy(sourcePath,
                 destinationPath,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing,
                 ec);
        if (ec) {
            LOG_ERROR("CommandLineEditor", "打包失败: {} ({})", sourcePath.string(), ec.message());
            return 1;
        }
    }

    LOG_INFO("CommandLineEditor", "打包完成: {}", packageDir.string());
    return 0;
}

int CommandLineEditor::CommandShowInfo(const std::vector<std::string>& args) {
    const fs::path projectRoot = ResolveProjectPath(m_args);
    const fs::path buildDir = ResolveOutputPath(m_args, projectRoot, "build");
    std::cout << "Project: " << projectRoot << "\n";
    std::cout << "Build: " << buildDir << "\n";
    std::cout << "Command: " << m_args.command << "\n";
    std::cout << "Mode: " << static_cast<int>(m_args.mode) << "\n";
    std::cout << "Has CMakeLists: " << fs::exists(projectRoot / "CMakeLists.txt") << "\n";
    std::cout << "Has assets: " << fs::exists(projectRoot / "assets") << "\n";
    if (!args.empty()) {
        std::cout << "Args:";
        for (const auto& arg : args) {
            std::cout << " " << arg;
        }
        std::cout << "\n";
    }
    return 0;
}

int CommandLineEditor::CommandValidate(const std::vector<std::string>& args) {
    const fs::path projectRoot = ResolveProjectPath(m_args);
    bool valid = fs::exists(projectRoot) && fs::is_directory(projectRoot);
    valid = valid && fs::exists(projectRoot / "CMakeLists.txt");

    for (const auto& arg : args) {
        if (!fs::exists(arg)) {
            LOG_ERROR("CommandLineEditor", "校验失败，缺少路径: {}", arg);
            valid = false;
        }
    }

    if (!valid) {
        LOG_ERROR("CommandLineEditor", "项目校验失败: {}", projectRoot.string());
        return 1;
    }

    LOG_INFO("CommandLineEditor", "项目校验通过: {}", projectRoot.string());
    return 0;
}

int CommandLineEditor::CommandRun(const std::vector<std::string>& args) {
    fs::path executablePath;
    if (!args.empty()) {
        executablePath = fs::path(args.front());
    } else {
        const fs::path projectRoot = ResolveProjectPath(m_args);
        const std::vector<fs::path> candidates = {
            projectRoot / "build/bin/Prisma",
            projectRoot / "build/bin/PrismaLauncher",
            projectRoot / "bin/Prisma",
            projectRoot / "bin/PrismaLauncher"
        };

        for (const auto& candidate : candidates) {
            if (fs::exists(candidate)) {
                executablePath = candidate;
                break;
            }
        }
    }

    if (executablePath.empty() || !fs::exists(executablePath)) {
        LOG_ERROR("CommandLineEditor", "未找到可执行文件");
        return 1;
    }

    return RunProcess(QuotePath(executablePath));
}

} // namespace Prisma
