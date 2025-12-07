/*
    Editor也相当于Game，但有自己的特殊逻辑
    Game为Runtime执行游戏逻辑
    Editor为Runtime执行编辑器逻辑
*/
#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#elif defined(__ANDROID__)
#include "ApplicationAndroid.h"
#elif defined(__linux__)
#include "ApplicationLinux.h"
#elif defined(__APPLE__)
#endif

#include "../core/Common.h"

#include "../core/DynamicLoader.h"
#include "Export.h"
#include <iostream>

// ============================================================================
// 应用程序入口点
// ============================================================================
int main(int argc, char* argv[]) {

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    // 初始化命令行参数解析器
    CommandLineParser cmdParser;

    // 添加运行时特定的命令行选项
#if defined(_WIN32) || defined(_WIN64)
    cmdParser.AddOption("fullscreen", "f", "以全屏模式启动", false);
    cmdParser.AddOption("width", "", "设置窗口宽度", true);
    cmdParser.AddOption("height", "", "设置窗口高度", true);
    cmdParser.AddOption("log-level", "l", "设置日志级别 (trace, debug, info, warning, error)", true);
    cmdParser.AddOption("project-path", "p", "设置项目路径", true);
    cmdParser.AddOption("log-file", "", "设置日志文件路径", true);
    cmdParser.AddOption("log-size", "", "设置日志文件大小", true);
    cmdParser.AddOption("log-count", "", "设置日志文件数量", true);

    cmdParser.AddOption("editor", "", "尝试启动编辑器", false);
    cmdParser.AddOption("game", "", "尝试启动游戏", false);

    // 添加动作选项示例
    cmdParser.AddActionOption("version", "V", "显示版本信息", false, [](const std::string&) {
        std::cout << "YAGE Runtime 版本 1.0.0" << std::endl;
        return true;  // 执行后退出
    });
#endif

    // 解析命令行参数
    auto parseResult = cmdParser.Parse(argc, argv);
    if (parseResult == CommandLineParser::ParseResult::Error) {
        return -1;
    }
    if (parseResult == CommandLineParser::ParseResult::ActionRequested) {
        return 0;  // 显示帮助或执行动作后正常退出
    }

    // 根据命令行参数配置日志
    LogConfig logConfig = LogConfig();
    // 如果是调试状态，禁用ANSI颜色代码
    // logConfig.enableColors = !isRunningInDebugger();
    
    if (cmdParser.IsOptionSet("log-file")) {
        logConfig.logFilePath = cmdParser.GetOptionValue("log-file");
    } else {
        logConfig.logFilePath = "logs/runtime.log";
    }
    if (cmdParser.IsOptionSet("log-size")) {
        logConfig.maxFileSize = std::stoull(cmdParser.GetOptionValue("log-size"));
    }
    if (cmdParser.IsOptionSet("log-count")) {
        logConfig.maxFileCount = std::stoull(cmdParser.GetOptionValue("log-count"));
    }


    // 根据命令行参数设置日志级别
    if (cmdParser.IsOptionSet("log-level")) {
        std::string levelStr = cmdParser.GetOptionValue("log-level");
        if (levelStr == "trace") {
            logConfig.minLevel = LogLevel::Trace;
        } else if (levelStr == "debug") {
            logConfig.minLevel = LogLevel::Debug;
        } else if (levelStr == "info") {
            logConfig.minLevel = LogLevel::Info;
        } else if (levelStr == "warning") {
            logConfig.minLevel = LogLevel::Warning;
        } else if (levelStr == "error") {
            logConfig.minLevel = LogLevel::Error;
        }
    }
    if (!Logger::GetInstance().Initialize(logConfig)) {
        LOG_FATAL("Logger", "日志系统初始化失败，正在退出...");
        return -1;
    }
    if (cmdParser.IsOptionSet("editor")) {
        LOG_INFO("Runtime", "尝试启动编辑器");
        return 0;
    }
    if (cmdParser.IsOptionSet("game")) {
        LOG_INFO("Runtime", "尝试启动游戏");
        return 0;
    }
    LOG_INFO("Runtime", "未指定启动模式");
    return 0;
    // 动态加载 Engine DLL
    DynamicLoader engineLoader;
    try {
        engineLoader.Load("Engine.dll");
        LOG_INFO("Runtime", "成功加载 Engine.dll");

    } catch (const std::exception& e) {
        LOG_FATAL("Runtime", "无法加载 Engine.dll: {}", e.what());
        return -1;
    }

    auto initialize = engineLoader.GetFunction<InitializeFunc>("Initialize");
    auto run = engineLoader.GetFunction<RunFunc>("Run");
    auto shutdown = engineLoader.GetFunction<ShutdownFunc>("Shutdown");

    LOG_INFO("Runtime", "获取 Application 实例成功");

    if (!initialize()) {
        LOG_FATAL("Runtime", "应用程序初始化失败，正在退出...");
        return -1;
    }
    LOG_INFO("Runtime", "Application 初始化成功");

    int exitCode = run();
    LOG_INFO("Runtime", "Application 运行完成，退出码: {}", exitCode);

    shutdown();
    LOG_INFO("Runtime", "Application 已关闭");

    Logger::GetInstance().Flush();  // 确保日志被写出
    return exitCode;
}