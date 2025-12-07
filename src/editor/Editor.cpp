/*
    Editor也相当于Game，但有自己的特殊逻辑
    Game为Runtime执行游戏逻辑
    Editor为Runtime执行编辑器逻辑
*/
#include "CommandLineParser.h"
#include "Logger.h"
#include "Common.h"
#include "ResourceManager.h"
#include "WindowsUtils.h"

// 全局变量用于热重载
DynamicLoader gameLoader;
// 文件监视器
Editor::DirectoryWatcher dllWatcher;
// DLL重载标志
std::mutex g_reloadMutex;

namespace Editor {

InitializeFunc initialize = nullptr;
RunFunc run               = nullptr;
UpdateFunc update         = nullptr;
ShutdownFunc shutdown     = nullptr;
}  // namespace Editor

// 重新加载DLL
bool ReloadEngineDLL() {
    LOG_INFO("Editor", "开始重新加载Engine.dll...");

    // 卸载旧的DLL
    if (gameLoader.IsLoaded()) {
        LOG_INFO("Editor", "卸载旧的Engine.dll...");
        if (Editor::shutdown) {
            Editor::shutdown();
        }
        gameLoader.Unload();
    }

    // 重新加载DLL
    try {
        if (!gameLoader.TryLoad("Engine.dll")) {
            LOG_ERROR("Editor", "无法加载新的Engine.dll");
            return false;
        }
        LOG_INFO("Editor", "成功加载新的Engine.dll");

        // 重新获取函数指针
        if (!gameLoader.TryGetFunction("InitializeEditor", Editor::initialize) ||
            !gameLoader.TryGetFunction("RunEditor", Editor::run) ||
            !gameLoader.TryGetFunction("ShutdownEditor", Editor::shutdown)) {
            LOG_ERROR("Editor", "无法获取必要的函数指针");
            gameLoader.Unload();
            return false;
        }

        LOG_INFO("Editor", "成功获取所有函数指针");

        // 初始化新DLL
        if (!Editor::initialize()) {
            LOG_ERROR("Editor", "新DLL初始化失败");
            gameLoader.Unload();
            return false;
        }

        LOG_INFO("Editor", "Engine.dll重新加载完成");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Editor", "重新加载Engine.dll时发生异常: {}", e.what());
        return false;
    }
}

// DLL更改回调函数
void OnDLLChanged(const std::wstring& filename, Editor::FileAction action) {
    // 只关注Engine.dll的更改
    if (filename == L"Engine.dll" && action == Editor::FileAction::Modified) {
        LOG_INFO("Editor", "检测到Engine.dll更改，请求重新加载...");
        std::lock_guard<std::mutex> lock(g_reloadMutex);
        // 重新加载DLL
        if (ReloadEngineDLL()) {
            LOG_INFO("Editor", "DLL重新加载成功");
        } else {
            LOG_ERROR("Editor", "DLL重新加载失败");
        }
    }
}


int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 初始化命令行参数解析器
    CommandLineParser& cmdParser = CommandLineParser::GetInstance();
    
    // 添加编辑器特定的命令行选项
    cmdParser.AddOption("verbose", "v", "启用详细日志", false);
    cmdParser.AddOption("project-path", "p", "指定项目路径", true);
    //cmdParser.AddActionOption(
    //    "create-default-assets", "", "创建默认资产(Mesh, Shader, Material等)", false, [](const std::string&) {
    //        ResourceManager::GetInstance()->CreateDefaultAssets();
    //        return true;
    //    });

    // 添加动作选项示例
    cmdParser.AddActionOption("version", "V", "显示版本信息", false, [](const std::string&) {
        LOG_INFO("Logger", "YAGE Editor 版本 1.0.0");
        return true;  // 显示帮助后退出
    });

    // 解析命令行参数
    auto parseResult = cmdParser.Parse(argc, argv);
    if (parseResult == CommandLineParser::ParseResult::Error) {
        return -1;
    }
    if (parseResult == CommandLineParser::ParseResult::ActionRequested) {
        return 0;  // 显示帮助或执行动作后正常退出
    }
    // 根据命令行参数调整日志级别
    LogConfig logConfig;
    if (cmdParser.IsOptionSet("verbose")) {
        logConfig.minLevel = LogLevel::Trace;
    }
    // 初始化日志系统
    Logger::GetInstance().Initialize(logConfig);
    if (logConfig.minLevel == LogLevel::Trace) {
        LOG_INFO("Logger", "已启用详细日志输出");
    }
    else {
        LOG_INFO("Logger", "使用的日志级别 : {0}", static_cast<int>(logConfig.minLevel));
    }

    // 启动DLL监视器
    std::filesystem::path currentPath = std::filesystem::current_path();
    if (!dllWatcher.Start(currentPath, OnDLLChanged)) {
        LOG_WARNING("Editor", "无法启动DLL监视器");
    } else {
        LOG_INFO("Editor", "DLL监视器已启动，监控路径: {}", currentPath.string());
    }

    // 动态加载 Editor.dll
    try {
        if (!gameLoader.TryLoad("Editor.dll")) {
            LOG_FATAL("Editor", "无法加载 Editor.dll");
            return -1;
        }
        LOG_INFO("Editor", "成功加载 Engine.dll");

    } catch (const std::exception& e) {
        LOG_FATAL("Editor", "无法加载 Engine.dll: {}", e.what());
        return -1;
    }

    if (!gameLoader.TryGetFunction("Initialize", Editor::initialize) ||
        !gameLoader.TryGetFunction("Run", Editor::run) || !gameLoader.TryGetFunction("Update", Editor::update) ||
        !gameLoader.TryGetFunction("Shutdown", Editor::shutdown)) {
        LOG_FATAL("Editor", "无法获取必要的函数指针");
        return -1;
    }

    LOG_INFO("Editor", "获取 Application 实例成功");

    if (!Editor::initialize()) {
        LOG_FATAL("Editor", "应用程序初始化失败，正在退出...");
        return -1;
    }
    LOG_INFO("Editor", "Application 初始化成功");

    // 主循环
    int exitCode = 0;

    // 运行应用程序逻辑
    exitCode = Editor::run();

    // 添加一个小的延迟以减少CPU使用率
    LOG_INFO("Editor", "Application 运行完成，退出码: {0}", exitCode);

    Editor::shutdown();
    LOG_INFO("Editor", "Application 已关闭");

    // 停止监视器
    dllWatcher.Stop();

    Logger::GetInstance().Flush();  // 确保日志被写出
    return exitCode;
}