#include "../engine/Logger.h"
#include <iostream>

#if defined(_WIN32)
    #define DLL_IMPORT __declspec(dllimport)
#else
    #define DLL_IMPORT
#endif

// 声明 DLL 导出的 C 接口
extern "C" {
    DLL_IMPORT int PrismaEditor_Main(int argc, char** argv, Logger* externalLogger);
}

int main(int argc, char* argv[]) {
    // 1. 获取宿主程序的日志系统实例
    Logger& logger = Logger::GetInstance();
    
    // 2. 将控制权交给 DLL 中的编辑器主逻辑
    return PrismaEditor_Main(argc, argv, &logger);
}
