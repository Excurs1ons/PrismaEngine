#pragma once
#include "Engine.h"
#include "Application.h"
#include "Logger.h"

#ifdef PRISMA_PLATFORM_WINDOWS

extern Prisma::Application* Prisma::CreateApplication();

int main(int argc, char** argv) {
    // 1. 初始化引擎配置 (可以根据命令行参数修改)
    Prisma::EngineSpecification spec;
    spec.Name = "Prisma App";
    
    // 2. 显式创建 Engine 对象
    // 这个对象存在于 stack 上，它的生命周期就是程序的生命周期。
    // 没有 shared_ptr，没有垃圾。
    Prisma::Engine engine(spec);
    
    // 3. 启动引擎
    if (engine.Initialize() != 0) {
        return -1;
    }

    // 4. 创建并运行 App
    // Engine 运行完 Run 之后会自己销毁。
    auto* app = Prisma::CreateApplication();
    int result = engine.Run(std::unique_ptr<Prisma::Application>(app));

    // 5. 显式关闭 (虽然析构函数也会处理，但显式调用能更好的控制顺序)
    engine.Shutdown();

    return result;
}

#endif
