#include "../../engine/CommandLineParser.h"
#include "../../engine/Common.h"

#include <cstdlib>
#include <string>

// 静态链接模式下由 Game 模块导出
extern "C" {
bool Game_Initialize();
int Game_Run();
void Game_Shutdown();
}

int main(int argc, char* argv[]) {
    auto& cmdParser = CommandLineParser::GetInstance();
    cmdParser.AddOption("assets-path", "a", "设置资源路径", true);

    auto parseResult = cmdParser.Parse(argc, argv);
    if (parseResult == CommandLineParser::ParseResult::Error) {
        return -1;
    }

    LogConfig logConfig{};
    logConfig.logFilePath = "/tmp/PrismaRuntime.log";
    logConfig.enableColors = false;

    if (!Logger::GetInstance().Initialize(logConfig)) {
        return -1;
    }

    std::string assetsPath = "/Assets";
    if (cmdParser.IsOptionSet("assets-path")) {
        assetsPath = cmdParser.GetOptionValue("assets-path");
    }

    setenv("PRISMA_ASSETS_PATH", assetsPath.c_str(), 1);

    if (!Game_Initialize()) {
        LOG_FATAL("Runtime", "Game 初始化失败");
        return -1;
    }

    const int exitCode = Game_Run();
    Game_Shutdown();
    Logger::GetInstance().Flush();
    return exitCode;
}
