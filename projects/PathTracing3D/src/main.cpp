/**
 * PathTracing3D — PrismaEngine 3D 路径追踪模板
 *
 * 演示 PrismaEngine 的路径追踪能力：
 *   - Compute-shader 路径追踪（Cornell Box 场景）
 *   - 五面立方体：左红右绿，顶部白色面光源
 *
 * 编译: cmake --build build --build --target PathTracing3D --config Debug
 * 运行: build/bin/Debug/PathTracing3D
 */

#include "PathTracing3DApp.h"
#include "app/EngineLauncher.h"
#include <iostream>
#include <string_view>
#include <cstdlib>

int main(int argc, char* argv[]) {
    Prisma::EngineSpecification spec;
    spec.Name = "PathTracing3D";
    spec.Headless = false;

    // CLI 参数直接映射到 EngineSpecification 字段
    // 0/空 = 使用 project.jsonc 值（Engine::Run 自动合并）
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--headless") {
            spec.Headless = true;
        } else if (arg == "--frames" && i + 1 < argc) {
            spec.HeadlessFrames = static_cast<uint32_t>(std::max(1, std::atoi(argv[++i])));
        } else if (arg == "--output" && i + 1 < argc) {
            spec.HeadlessOutputPath = argv[++i];
        } else if (arg == "--width" && i + 1 < argc) {
            spec.HeadlessWidth = static_cast<uint32_t>(std::max(1, std::atoi(argv[++i])));
        } else if (arg == "--height" && i + 1 < argc) {
            spec.HeadlessHeight = static_cast<uint32_t>(std::max(1, std::atoi(argv[++i])));
        } else if (arg == "--samples" && i + 1 < argc) {
            spec.MaxSamples = static_cast<uint32_t>(std::max(1, std::atoi(argv[++i])));
        }
    }

    auto app = std::make_unique<Prisma::PathTracing3DApp>();

    if (spec.Headless) {
        std::cout << "PathTracing3D headless mode: frames=" << spec.HeadlessFrames
                  << " output=" << spec.HeadlessOutputPath
                  << " " << (spec.HeadlessWidth ? std::to_string(spec.HeadlessWidth) : "?") << "x" << (spec.HeadlessHeight ? std::to_string(spec.HeadlessHeight) : "?") << std::endl;
    }

    int result = Prisma::RunApplication(
        std::move(app),
        spec
    );

    if (result != 0) {
        std::cerr << "PathTracing3D exited with error: " << result << std::endl;
    }

    return result;
}
