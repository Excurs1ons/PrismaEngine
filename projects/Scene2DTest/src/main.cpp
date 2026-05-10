/**
 * Scene2DTest — PrismaEngine 2D 场景模板
 *
 * 最小化 2D 渲染示例，演示 Renderer2D API 的基本用法：
 *   - OrthographicCamera 设置
 *   - Renderer2D::BeginScene / EndScene
 *   - Renderer2D::DrawQuad (彩色方块、旋转)
 *   - Renderer2D::DrawString (文字渲染)
 *
 * 编译: cmake --build build --target Scene2DTest --config Debug
 * 运行: build/bin/Debug/Scene2DTest.exe  (or via pacman preset build dir)
 */

#include "Scene2DTestApp.h"
#include "app/EngineLauncher.h"
#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    Prisma::EngineSpecification spec;
    spec.Name = "Scene2DTest";
    spec.Headless = false;

    bool autoQuit = false;

    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--headless") {
            spec.Headless = true;
        } else if (arg == "--quit") {
            autoQuit = true;
        }
    }

    spec.RefreshAssetDatabaseOnStartup = false;
    spec.MaxFPS = 0; // 设为 0 以解除帧率限制

    auto app = std::make_unique<Prisma::Scene2DTestApp>();
    app->SetAutoQuit(autoQuit);

    int result = Prisma::RunApplication(
        std::move(app),
        spec
    );

    if (result != 0) {
        std::cerr << "Scene2DTest exited with error: " << result << std::endl;
    }

    return result;
}
