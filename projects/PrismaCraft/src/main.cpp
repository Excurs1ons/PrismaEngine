#include "PrismaCraftApp.h"
#include "app/EngineLauncher.h"
#include <iostream>

int main(int argc, char* argv[]) {
    Prisma::EngineSpecification spec;
    spec.Name = "PrismaCraft";
    spec.Headless = false;
    spec.RefreshAssetDatabaseOnStartup = false;

    auto app = std::make_unique<Prisma::PrismaCraftApp>();
    int result = Prisma::RunApplication(std::move(app), spec);

    if (result != 0)
        std::cerr << "PrismaCraft exited with error: " << result << std::endl;
    return result;
}
