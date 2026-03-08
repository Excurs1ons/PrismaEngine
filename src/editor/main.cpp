#include "../engine/Logger.h"
#include <iostream>

#if defined(_WIN32)
    #define DLL_IMPORT __declspec(dllimport)
#else
    #define DLL_IMPORT
#endif

extern "C" {
    DLL_IMPORT int PrismaEditor_Main(int argc, char** argv, Logger* externalLogger);
}

int main(int argc, char* argv[]) {
    // 1. 获取并初始化日志系统
    Logger& logger = Logger::GetInstance();
    
    LogConfig config;
    config.target = LogTarget::Both;
    config.asyncMode = false; // 关键：在调试初始化问题时使用同步模式
    logger.Initialize(config);

    LOG_INFO("Launcher", "PrismaEditor 启动器已就绪...");

    // 2. 调用 DLL 中的主函数
    int exitCode = PrismaEditor_Main(argc, argv, &logger);

    // 3. 强制刷新并关闭
    LOG_INFO("Launcher", "程序即将退出，退出码: {}", exitCode);
    logger.Flush();
    logger.Shutdown();

    return exitCode;
}
