/**
 * MetroidvaniaDemo — NES-style Metroidvania tech demo on PrismaEngine
 *
 * 1024×896 窗口 (256×224 ×4 pixel-perfect), CoreCLR 脚本化游戏逻辑
 */

#include "MetroidvaniaApp.h"
#include "app/EngineLauncher.h"
#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    Prisma::EngineSpecification spec;
    spec.Name = "MetroidvaniaDemo";
    spec.Headless = false;

    bool autoQuit = false;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--headless") {
            spec.Headless = true;
        } else if (arg == "--quit") {
            autoQuit = true;
        }
    }

    spec.RefreshAssetDatabaseOnStartup = false;
    auto app = std::make_unique<Prisma::MetroidvaniaApp>();
    app->SetAutoQuit(autoQuit);

    int result = Prisma::RunApplication(std::move(app), spec);

    if (result != 0) {
        std::cerr << "MetroidvaniaDemo exited with error: " << result << std::endl;
    }

    return result;
}
