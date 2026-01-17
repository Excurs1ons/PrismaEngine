//
// Linux Runtime
//
#if defined(__linux__) || defined(__LINUX__) || defined(LINUX)

#include "../../engine/Common.h"
#include "../../engine/DynamicLoader.h"
#include "Platform.h"
#include <chrono>
#include <iostream>
#include <thread>

// 函数指针类型定义
using InitializeFunc = bool (*)();
using RunFunc = int (*)();
using ShutdownFunc = void (*)();

// ============================================================================
// 应用程序入口点
// ============================================================================
int main(int argc, char* argv[]) {
    // 初始化命令行参数解析器
    CommandLineParser cmdParser;

    // 添加运行时特定的命令行选项
    cmdParser.AddOption("fullscreen", "f", "以全屏模式启动", false);
    cmdParser.AddOption("width", "", "设置窗口宽度", true);
    cmdParser.AddOption("height", "", "设置窗口高度", true);
    cmdParser.AddOption("log-level", "l", "设置日志级别 (trace, debug, info, warning, error)", true);
    cmdParser.AddOption("project-path", "p", "设置项目路径", true);
    cmdParser.AddOption("assets-path", "a", "设置资源路径", true);
    cmdParser.AddOption("log-file", "", "设置日志文件路径", true);
    cmdParser.AddOption("log-size", "", "设置日志文件大小", true);
    cmdParser.AddOption("log-count", "", "设置日志文件数量", true);
    cmdParser.AddOption("editor", "", "尝试启动编辑器", false);
    cmdParser.AddOption("game", "", "尝试启动游戏", false);
    cmdParser.AddOption("test-render", "t", "运行新渲染系统测试", false);

    // 添加动作选项示例
    cmdParser.AddActionOption("version", "V", "显示版本信息", false, [](const std::string&) {
        std::cout << "Prisma Runtime 版本 1.0.0" << '\n';
        return true;
    });

    // 解析命令行参数
    auto parseResult = cmdParser.Parse(argc, argv);
    if (parseResult == CommandLineParser::ParseResult::Error) {
        return -1;
    }
    if (parseResult == CommandLineParser::ParseResult::ActionRequested) {
        return 0;
    }

    // 根据命令行参数配置日志
    LogConfig logConfig = LogConfig();
#ifdef _DEBUG
    logConfig.enableCallStack = true;
#endif

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
        LOG_FATAL("Runtime", "日志系统初始化失败，正在退出...");
        return -1;
    }

    // Linux 动态库文件名 (.so)
    const char* lib_name;
    if (cmdParser.IsOptionSet("editor")) {
        LOG_INFO("Runtime", "尝试启动编辑器");
        lib_name = "libPrismaEditor.so";
    } else {
        LOG_INFO("Runtime", "默认启动游戏模式");
        lib_name = "libPrismaGame.so";
    }

    // 设置资源路径
    std::string assetsPath;
    if (cmdParser.IsOptionSet("assets-path")) {
        assetsPath = cmdParser.GetOptionValue("assets-path");
        LOG_INFO("Runtime", "使用指定的资源路径: {0}", assetsPath);
    } else if (cmdParser.IsOptionSet("project-path")) {
        assetsPath = cmdParser.GetOptionValue("project-path") + "/assets";
        LOG_INFO("Runtime", "使用项目路径下的资源目录: {0}", assetsPath);
    } else {
        assetsPath = "./Assets";
        LOG_INFO("Runtime", "使用默认资源路径: {0}", assetsPath);
    }

    // 设置环境变量
    setenv("PRISMA_ASSETS_PATH", assetsPath.c_str(), 1);

    int exit_code = 0;  // 提前声明，避免作用域问题

#ifdef PRISMA_STATIC_LINKED_GAME
    // 静态链接模式 - 直接调用函数
    extern "C" {
        bool Game_Initialize();
        int Game_Run();
        void Game_Shutdown();
    }

    LOG_INFO("Runtime", "静态链接模式 - 直接调用 Game 模块");

    if (!Game_Initialize()) {
        LOG_FATAL("Runtime", "应用程序初始化失败，正在退出...");
        return -1;
    }
    LOG_INFO("Runtime", "Game 初始化成功");

    exit_code = Game_Run();
    LOG_INFO("Runtime", "Game 运行完成，退出码: {0}", exit_code);

    Game_Shutdown();
    LOG_INFO("Runtime", "Game 已关闭");
#else
    // 动态库模式 - 使用 dlopen 加载 .so
    LOG_INFO("Runtime", "动态库模式 - 加载 {0}", lib_name);

    DynamicLoader game_loader;
    try {
        game_loader.Load(lib_name);
        LOG_INFO("Runtime", "成功加载 {0}", lib_name);
    } catch (const std::exception& e) {
        LOG_FATAL("Runtime", "无法加载 {0}: {1}", lib_name, e.what());
        return -1;
    }

    auto initialize = game_loader.GetFunction<InitializeFunc>("Initialize");
    auto run = game_loader.GetFunction<RunFunc>("Run");
    auto shutdown = game_loader.GetFunction<ShutdownFunc>("Shutdown");

    LOG_INFO("Runtime", "获取 {0} 实例成功", lib_name);

    if (!initialize()) {
        LOG_FATAL("Runtime", "应用程序初始化失败，正在退出...");
        return -1;
    }
    LOG_INFO("Runtime", "{0} 初始化成功", lib_name);

    exit_code = run();
    LOG_INFO("Runtime", "{0} 运行完成，退出码: {1}", lib_name, exit_code);

    shutdown();
    LOG_INFO("Runtime", "{0} 已关闭", lib_name);
#endif

    Logger::GetInstance().Flush();
    return exit_code;
}

#endif
