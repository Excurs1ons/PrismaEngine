/**
 * Template3D — PrismaEngine 3D 路径追踪模板
 *
 * 演示 PrismaEngine 的路径追踪能力：
 *   - Compute-shader 路径追踪（Cornell Box 场景）
 *   - 五面立方体：左红右绿，顶部白色面光源
 *
 * 编译: cmake --build build --target Template3D --config Debug
 * 运行: build/bin/Debug/Template3D
 */

#include "Template3DApp.h"
#include "app/EngineLauncher.h"
#include <iostream>
#include <string_view>
#include <cstdlib>

int main(int argc, char* argv[]) {
    Prisma::EngineSpecification spec;
    spec.Name = "Template3D";
    spec.Headless = false;

    bool autoQuit = false;
    uint32_t headlessFrames = 0;     // 0 = 从 project.json 读取
    uint32_t headlessWidth = 0;
    uint32_t headlessHeight = 0;
    uint32_t samples = 0;            // 0 = 从 project.json 读取
    std::string outputPath;          // 空 = 从 project.json 读取

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--headless") {
            spec.Headless = true;
        } else if (arg == "--quit") {
            autoQuit = true;
        } else if (arg == "--frames" && i + 1 < argc) {
            headlessFrames = static_cast<uint32_t>(std::max(1, std::atoi(argv[++i])));
        } else if (arg == "--output" && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (arg == "--width" && i + 1 < argc) {
            headlessWidth = static_cast<uint32_t>(std::max(1, std::atoi(argv[++i])));
        } else if (arg == "--height" && i + 1 < argc) {
            headlessHeight = static_cast<uint32_t>(std::max(1, std::atoi(argv[++i])));
        } else if (arg == "--samples" && i + 1 < argc) {
            samples = static_cast<uint32_t>(std::max(1, std::atoi(argv[++i])));
        }
    }

    auto app = std::make_unique<Prisma::Template3DApp>();
    app->SetAutoQuit(autoQuit);
    app->SetSamples(samples);
    if (spec.Headless) {
        app->SetHeadlessConfig(headlessFrames, outputPath, headlessWidth, headlessHeight);
        std::cout << "Template3D 头模式: frames=" << headlessFrames
                  << " output=" << outputPath
                  << " " << (headlessWidth ? std::to_string(headlessWidth) : "?") << "x" << (headlessHeight ? std::to_string(headlessHeight) : "?") << std::endl;
    }

    int result = Prisma::RunApplication(
        std::move(app),
        spec
    );

    if (result != 0) {
        std::cerr << "Template3D exited with error: " << result << std::endl;
    }

    return result;
}
