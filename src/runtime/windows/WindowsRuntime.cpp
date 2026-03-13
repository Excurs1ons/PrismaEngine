/*
    PrismaEngine Runtime 始终动态加载 Game/Editor
    Game 为游戏逻辑模块
    Editor 为编辑器逻辑模块
*/
#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#endif

#include "../../engine/CommandLineParser.h"
#include "../../engine/Common.h"
#include "../../engine/DynamicLoader.h"
#include "Export.h"
#include "Platform.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <cstdlib>

// ============================================================================
// 常量定义
// ============================================================================
namespace {
constexpr const char* EDITOR_LIB = "PrismaEditor.dll";
constexpr const char* GAME_LIB   = "PrismaGame.dll";
}  // namespace

// 函数指针类型定义
using InitializeFunc = bool (*)();
using RunFunc        = int (*)();
using ShutdownFunc   = void (*)();

// ============================================================================
// 应用程序入口点
// ============================================================================
int main(int argc, char* argv[]) {
#if defined(_WIN32) || defined(_WIN64)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    auto& cmdParser = CommandLineParser::GetInstance();

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

    cmdParser.AddActionOption("version", "V", "显示版本信息", false, [](const std::string&) {
        std::cout << "PrismaEngine Runtime 版本 1.0.0\n";
        return true; 
    });

    auto parseResult = cmdParser.Parse(argc, argv);
    if (parseResult == CommandLineParser::ParseResult::Error) {
        return -1;
    }
    if (parseResult == CommandLineParser::ParseResult::ActionRequested) {
        return 0; 
    }

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
        const std::string& sizeStr = cmdParser.GetOptionValue("log-size");
        char* end;
        unsigned long long size = std::strtoull(sizeStr.c_str(), &end, 10);
        if (end != sizeStr.c_str()) {
            logConfig.maxFileSize = size;
        }
    }
    
    if (cmdParser.IsOptionSet("log-count")) {
        const std::string& countStr = cmdParser.GetOptionValue("log-count");
        char* end;
        unsigned long long count = std::strtoull(countStr.c_str(), &end, 10);
        if (end != countStr.c_str()) {
            logConfig.maxFileCount = count;
        }
    }

    if (cmdParser.IsOptionSet("log-level")) {
        const std::string& levelStr = cmdParser.GetOptionValue("log-level");
        if (levelStr == "trace") logConfig.minLevel = LogLevel::Trace;
        else if (levelStr == "debug") logConfig.minLevel = LogLevel::Debug;
        else if (levelStr == "info") logConfig.minLevel = LogLevel::Info;
        else if (levelStr == "warning") logConfig.minLevel = LogLevel::Warning;
        else if (levelStr == "error") logConfig.minLevel = LogLevel::Error;
    }

    if (!Logger::GetInstance().Initialize(logConfig)) {
        std::cerr << "日志系统初始化失败，正在退出...\n";
        return -1;
    }

    const char* lib_name = cmdParser.IsOptionSet("editor") ? EDITOR_LIB : GAME_LIB;
    LOG_INFO("Runtime", "启动模式: 加载 {0}", lib_name);

    std::string assetsPath = "./Assets";
    if (cmdParser.IsOptionSet("assets-path")) {
        assetsPath = cmdParser.GetOptionValue("assets-path");
    } else if (cmdParser.IsOptionSet("project-path")) {
        assetsPath = cmdParser.GetOptionValue("project-path") + "/assets";
    }
    LOG_INFO("Runtime", "资源路径: {0}", assetsPath);

#ifdef _WIN32
    SetEnvironmentVariableA("PRISMA_ASSETS_PATH", assetsPath.c_str());
#else
    setenv("PRISMA_ASSETS_PATH", assetsPath.c_str(), 1);
#endif

    DynamicLoader game_loader;
    try {
        game_loader.Load(lib_name);
        LOG_INFO("Runtime", "成功加载 {0}", lib_name);
    } catch (const std::exception& e) {
        LOG_FATAL("Runtime", "无法加载 {0}: {1}", lib_name, e.what());
        return -1;
    }

    InitializeFunc initialize = nullptr;
    RunFunc run = nullptr;
    ShutdownFunc shutdown = nullptr;

    if (!game_loader.TryGetFunction("Initialize", initialize) ||
        !game_loader.TryGetFunction("Run", run) ||
        !game_loader.TryGetFunction("Shutdown", shutdown)) {
        LOG_FATAL("Runtime", "缺少必要的导出函数 (Initialize/Run/Shutdown)，DLL 无效！");
        return -1;
    }

    if (!initialize()) {
        LOG_FATAL("Runtime", "应用程序初始化失败，正在退出...");
        return -1;
    }
    LOG_INFO("Runtime", "{0} 初始化成功", lib_name);

    int exit_code = run();
    LOG_INFO("Runtime", "{0} 运行完成，退出码: {1}", lib_name, exit_code);

    shutdown();
    LOG_INFO("Runtime", "{0} 已关闭", lib_name);

    Logger::GetInstance().Flush();
    return exit_code;
}
