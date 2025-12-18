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

#include "../engine/Common.h"

#include "../engine/DynamicLoader.h"
#include "Export.h"
#include "../engine/graphic/RenderSystemTest.h"
#include "PlatformWindows.h"
#include <iostream>
#include <thread>
#include <chrono>

// ============================================================================
// 函数声明
// ============================================================================
int RunRenderTest();

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
    cmdParser.AddOption("assets-path", "a", "设置资源路径", true);
    cmdParser.AddOption("log-file", "", "设置日志文件路径", true);
    cmdParser.AddOption("log-size", "", "设置日志文件大小", true);
    cmdParser.AddOption("log-count", "", "设置日志文件数量", true);

    cmdParser.AddOption("editor", "", "尝试启动编辑器", false);
    cmdParser.AddOption("game", "", "尝试启动游戏", false);
    cmdParser.AddOption("test-render", "t", "运行新渲染系统测试", false);

    // 添加动作选项示例
    cmdParser.AddActionOption("version", "V", "显示版本信息", false, [](const std::string&) {
        std::cout << "YAGE Runtime 版本 1.0.0" << '\n';
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
        LOG_FATAL("Logger", "日志系统初始化失败，正在退出...");
        return -1;
    }
    std::string lib_name;
    if (cmdParser.IsOptionSet("test-render")) {
        LOG_INFO("Runtime", "运行新渲染系统测试");
        // 直接运行渲染测试，不加载DLL
        return RunRenderTest();
    }
    else if (cmdParser.IsOptionSet("editor")) {
        LOG_INFO("Runtime", "尝试启动编辑器");
        lib_name = "PrismaEditor.dll";
    }
    else {
        // 默认启动游戏模式（如果没有指定--editor参数）
        LOG_INFO("Runtime", "默认启动游戏模式");
        lib_name = "PrismaGame.dll";
    }

    // 设置资源路径
    std::string assetsPath;
    if (cmdParser.IsOptionSet("assets-path")) {
        assetsPath = cmdParser.GetOptionValue("assets-path");
        LOG_INFO("Runtime", "使用指定的资源路径: {0}", assetsPath);
    } else if (cmdParser.IsOptionSet("project-path")) {
        // 如果指定了项目路径，资源路径应该是项目路径下的 assets
        assetsPath = cmdParser.GetOptionValue("project-path") + "/assets";
        LOG_INFO("Runtime", "使用项目路径下的资源目录: {0}", assetsPath);
    } else {
        // 默认路径：当前目录下的 Assets
        assetsPath = "./Assets";
        LOG_INFO("Runtime", "使用默认资源路径: {0}", assetsPath);
    }

    // 设置环境变量，让 ResourceManager 能找到资源
#ifdef _WIN32
    SetEnvironmentVariableA("PRISMA_ASSETS_PATH", assetsPath.c_str());
#else
    setenv("PRISMA_ASSETS_PATH", assetsPath.c_str(), 1);
#endif

    // 动态加载 Engine DLL
    DynamicLoader game_loader;
    try {
        game_loader.Load(lib_name);
        LOG_INFO("Runtime", "成功加载 {0}",lib_name);

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

    int exit_code = run();
    LOG_INFO("Runtime", "{0} 运行完成，退出码: {1}", lib_name, exit_code);

    shutdown();
    LOG_INFO("Runtime", "{0} 已关闭", lib_name);

    Logger::GetInstance().Flush();  // 确保日志被写出
    return exit_code;
}

// ============================================================================
// 新渲染系统测试函数
// ============================================================================
int RunRenderTest() {
    LOG_INFO("Runtime", "开始新渲染系统测试");

    try {
        // 创建测试实例
        PrismaEngine::Graphic::RenderSystemTest test;

        // 使用Platform系统创建测试窗口
        uint32_t width = 800;
        uint32_t height = 600;

#if defined(_WIN32)
        // 初始化Platform系统
        auto platform = PlatformWindows::GetInstance();
        if (!platform || !platform->Initialize()) {
            LOG_ERROR("Runtime", "无法初始化Platform系统");
            return -1;
        }

        // 创建窗口属性
        Engine::WindowProps windowProps("新渲染系统测试 - 彩色三角形", width, height);
        windowProps.Resizable = false;
        windowProps.ShowState = Engine::WindowShowState::Show;

        // 创建窗口
        auto windowHandle = platform->CreateWindow(windowProps);
        if (!windowHandle) {
            LOG_ERROR("Runtime", "无法创建测试窗口");
            return -1;
        }

        LOG_INFO("Runtime", "使用Platform创建测试窗口: {0}x{1}", width, height);
        LOG_INFO("Runtime", "窗口标题: {0}", windowProps.Title);
#else
        LOG_ERROR("Runtime", "渲染测试仅在Windows平台上支持");
        return -1;
#endif

        // 初始化测试
        if (!test.Initialize(windowHandle, width, height)) {
            LOG_ERROR("Runtime", "渲染系统测试初始化失败");
            return -1;
        }

        // 运行测试
        bool allTestsPassed = test.RunTests();

        if (allTestsPassed) {
            LOG_INFO("Runtime", "新渲染系统测试完成 - 所有测试通过");

            // 运行渲染测试进行可视化验证
            LOG_INFO("Runtime", "=== 开始渲染流程验证 - 可视化测试 ===");
            LOG_INFO("Runtime", "窗口将显示5秒，您可以查看渲染的三角形");

            // 渲染更多帧并处理窗口消息，让用户能看到结果
            for (int i = 0; i < 300; ++i) {  // 约5秒，60FPS
                test.RenderFrame();

#if defined(_WIN32)
                // 使用Platform系统处理窗口消息，确保窗口能正常显示和响应
                if (platform) {
                    platform->PumpEvents();

                    // 检查窗口是否应该关闭
                    if (platform->ShouldClose(windowHandle)) {
                        LOG_INFO("Runtime", "用户关闭了测试窗口");
                        break;
                    }
                }
#endif

                // 控制帧率约为60FPS
                std::this_thread::sleep_for(std::chrono::milliseconds(16));

                // 每60帧（约1秒）输出一次进度
                if (i % 60 == 0) {
                    LOG_INFO("Runtime", "渲染进度: {0}/300 帧 ({1:.1f}秒)", i + 1, (i + 1) / 60.0f);
                }
            }
            LOG_INFO("Runtime", "=== 渲染流程验证完成 ===");

            // 清理
            test.Shutdown();

#if defined(_WIN32)
            // 使用Platform销毁测试窗口
            if (platform && windowHandle) {
                platform->DestroyWindow(windowHandle);
                LOG_INFO("Runtime", "测试窗口已销毁");
            }
#endif

            return 0;
        } else {
            LOG_ERROR("Runtime", "新渲染系统测试失败");
            test.Shutdown();

#if defined(_WIN32)
            // 使用Platform销毁测试窗口
            if (platform && windowHandle) {
                platform->DestroyWindow(windowHandle);
                LOG_INFO("Runtime", "测试窗口已销毁");
            }
#endif

            return -1;
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Runtime", "渲染系统测试异常: {0}", e.what());

#if defined(_WIN32)
        // 使用Platform销毁测试窗口（如果存在）
        if (platform && windowHandle) {
            platform->DestroyWindow(windowHandle);
            LOG_INFO("Runtime", "测试窗口已销毁（异常清理）");
        }
#endif

        return -1;
    }
}