#include "Editor.h"
#include "CommandLineEditor.h"
#include "CommandLineParser.h"
#include "Environment.h"
#include "../engine/Logger.h"
#include <iostream>
#include <memory>

using namespace PrismaEngine;

/// <summary>
/// 编辑器主入口点
/// 自动检测运行环境并选择合适的运行模式：
/// - GUI: 图形界面模式（有显示系统）
/// - CLI: 命令行模式（无显示系统）
/// - Batch: 批处理模式（通过 --batch 参数指定）
/// - Server: 服务器模式（通过 --server 参数指定）
/// </summary>
int main(int argc, char* argv[]) {
    // 初始化日志系统
    Logger::GetInstance().Initialize();

    // 解析命令行参数
    CommandLineParser parser(argc, argv);
    if (!parser.Parse()) {
        LOG_FATAL("Main", "命令行参数解析失败");
        return 1;
    }

    const auto& args = parser.GetArguments();

    // 显示帮助或版本
    if (parser.ShouldShowInfo()) {
        if (args.options.find("help") != args.options.end()) {
            parser.ShowHelp();
        } else if (args.options.find("version") != args.options.end()) {
            parser.ShowVersion();
        }
        return 0;
    }

    // 自动检测运行环境
    EnvironmentType env = Environment::DetectEnvironment();
    LOG_INFO("Main", "检测到环境: {}", Environment::GetEnvironmentDescription());

    // 根据环境和参数选择编辑器类型
    IApplicationBase* editor = nullptr;
    EditorRunMode runMode = args.mode;

    // 如果用户明确指定了模式，使用用户指定的模式
    if (runMode == EditorRunMode::CLI || runMode == EditorRunMode::Batch) {
        // 用户明确要求命令行模式
        LOG_INFO("Main", "使用命令行模式（用户指定）");
        auto& cliEditor = CommandLineEditor::GetInstance();
        cliEditor.SetArguments(args);
        editor = &cliEditor;
    } else if (runMode == EditorRunMode::Server) {
        // 服务器模式（暂未实现）
        LOG_ERROR("Main", "服务器模式暂未实现");
        return 1;
    } else {
        // 用户未指定模式，自动选择
        if (env == EnvironmentType::Desktop && Environment::HasDisplaySupport()) {
            // 有显示支持，使用 GUI 模式
            LOG_INFO("Main", "启动图形界面编辑器");
            editor = &Editor::GetInstance();
        } else {
            // 无显示支持，自动切换到 CLI 模式
            LOG_INFO("Main", "未检测到显示系统，自动切换到命令行模式");
            auto& cliEditor = CommandLineEditor::GetInstance();
            cliEditor.SetArguments(args);
            editor = &cliEditor;
        }
    }

    // 初始化编辑器
    if (!editor || !editor->Initialize()) {
        LOG_FATAL("Main", "编辑器初始化失败");
        return 1;
    }

    // 运行编辑器
    int exitCode = editor->Run();

    // 关闭编辑器
    editor->Shutdown();

    return exitCode;
}
