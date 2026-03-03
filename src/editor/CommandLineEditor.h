#pragma once
#include "Export.h"
#include "IApplication.h"
#include "CommandLineParser.h"
#include <string>
#include <unordered_map>
#include <functional>

namespace PrismaEngine {

/// <summary>
/// 命令行编辑器 - 无窗口模式
/// 支持通过命令行执行编辑器功能
/// </summary>
class EDITOR_API CommandLineEditor : public IApplication<CommandLineEditor> {
public:
    friend class IApplication;
    CommandLineEditor();
    ~CommandLineEditor() override;

    bool Initialize() override;
    int Run() override;
    void Shutdown() override;

    /// <summary>
    /// 设置命令行参数
    /// </summary>
    void SetArguments(const CommandLineParser::Arguments& args);

    /// <summary>
    /// 注册命令处理器
    /// </summary>
    using CommandHandler = std::function<int(const std::vector<std::string>&)>;
    void RegisterCommand(const std::string& name, const std::string& description, CommandHandler handler);

private:
    CommandLineParser::Arguments m_args;

    struct CommandInfo {
        std::string description;
        CommandHandler handler;
    };
    std::unordered_map<std::string, CommandInfo> m_commands;

    // 内置命令
    int CommandBuild(const std::vector<std::string>& args);
    int CommandClean(const std::vector<std::string>& args);
    int CommandExport(const std::vector<std::string>& args);
    int CommandImport(const std::vector<std::string>& args);
    int CommandPackage(const std::vector<std::string>& args);
    int CommandShowInfo(const std::vector<std::string>& args);
    int CommandValidate(const std::vector<std::string>& args);
    int CommandRun(const std::vector<std::string>& args);

    void RegisterBuiltinCommands();
    void ShowHelp();
    int ExecuteCommand();
};

} // namespace PrismaEngine
