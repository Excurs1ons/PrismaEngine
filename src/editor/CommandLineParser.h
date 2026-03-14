#pragma once
#include "Export.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Prisma {

/// <summary>
/// 命令行编辑器运行模式
/// </summary>
enum class EditorRunMode {
    GUI,        // 默认 GUI 模式
    CLI,        // 命令行模式（无窗口）
    Batch,      // 批处理模式
    Server      // 服务器模式
};

/// <summary>
/// 命令行命令处理函数类型
/// </summary>
using CommandLineHandler = std::function<int(const std::vector<std::string>&)>;

/// <summary>
/// 命令行参数解析器
/// 支持以下格式：
///   -editor --mode cli --command build --project /path/to/project
///   -editor --batch --script build_assets.lua
///   -editor --server --port 8080
/// </summary>
class EDITOR_API CommandLineParser {
public:
    struct Arguments {
        EditorRunMode mode = EditorRunMode::GUI;
        std::string projectPath;
        std::string command;
        std::vector<std::string> commandArgs;
        std::unordered_map<std::string, std::string> options;

        // 常用选项
        bool verbose = false;
        bool quiet = false;
        int logLevel = 2; // 0=Trace, 1=Debug, 2=Info, 3=Warning, 4=Error, 5=Fatal
        std::string logFile;
        std::string outputPath;

        // 服务器模式选项
        int serverPort = 8080;

        // 批处理模式选项
        std::string scriptFile;
        bool continueOnError = false;
    };

    CommandLineParser(int argc, char* argv[]);
    ~CommandLineParser() = default;

    /// <summary>
    /// 解析命令行参数
    /// </summary>
    bool Parse();

    /// <summary>
    /// 获取解析后的参数
    /// </summary>
    const Arguments& GetArguments() const { return m_args; }

    /// <summary>
    /// 注册命令处理器
    /// </summary>
    void RegisterCommand(const std::string& name, const std::string& description, CommandLineHandler handler);

    /// <summary>
    /// 执行命令
    /// </summary>
    int ExecuteCommand();

    /// <summary>
    /// 显示帮助信息
    /// </summary>
    void ShowHelp() const;

    /// <summary>
    /// 显示版本信息
    /// </summary>
    void ShowVersion() const;

    /// <summary>
    /// 检查是否需要显示帮助或版本
    /// </summary>
    bool ShouldShowInfo() const;

    static const char* GetVersion();
    static const char* GetBuildInfo();

private:
    int m_argc;
    char** m_argv;
    Arguments m_args;

    struct CommandInfo {
        std::string name;
        std::string description;
        CommandLineHandler handler;
    };
#pragma warning(push)
#pragma warning(disable: 4251)
    std::vector<CommandInfo> m_commands;
#pragma warning(pop)

    bool ParseOption(const std::string& arg);
    std::string GetNextValue(size_t& index);
};

} // namespace Prisma
