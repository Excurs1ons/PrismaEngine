#include "Deferred3DApp.h"
#include "app/EngineLauncher.h"
#include <iostream>
#include <string_view>
#include <cstdlib>

int main(int argc, char* argv[])
{
    Prisma::EngineSpecification spec;
    spec.Name = "Deferred3D";
    spec.Headless = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--headless") {
            spec.Headless = true;
        } else if (arg == "--width" && i + 1 < argc) {
            spec.HeadlessWidth = static_cast<uint32_t>(std::max(1, std::atoi(argv[++i])));
        } else if (arg == "--height" && i + 1 < argc) {
            spec.HeadlessHeight = static_cast<uint32_t>(std::max(1, std::atoi(argv[++i])));
        }
    }

    auto app = std::make_unique<Prisma::Deferred3DApp>();

    int result = Prisma::RunApplication(
        std::move(app),
        spec
    );

    if (result != 0) {
        std::cerr << "Deferred3D exited with error: " << result << std::endl;
    }

    return result;
}
