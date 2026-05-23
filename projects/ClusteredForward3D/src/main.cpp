#include "ClusteredApp.h"
#include "app/EngineLauncher.h"
#include <memory>

int main(int argc, char* argv[]) {
    Prisma::EngineSpecification spec;
    spec.Name = "ClusteredForward3D";
    spec.Headless = false;

    auto app = std::make_unique<Prisma::ClusteredApp>();

    return Prisma::RunApplication(
        std::move(app),
        spec
    );
}
