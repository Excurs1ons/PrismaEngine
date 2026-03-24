#include "Logger.h"
#include <filesystem>
#include <fstream>

namespace Prisma {

class Packaging {
public:
    static bool PackageProject(const std::filesystem::path& targetDir) {
        LOG_INFO("Packaging", "Starting project packaging to {0}...", targetDir.string());
        
        try {
            if (!std::filesystem::exists(targetDir)) {
                std::filesystem::create_directories(targetDir);
            }

            // 1. 拷贝可执行文件 (假设在当前 build/bin)
            std::filesystem::copy("bin/PrismaLauncher", targetDir / "PrismaGame", std::filesystem::copy_options::overwrite_existing);
            
            // 2. 拷贝必要的共享库
            std::filesystem::copy("lib", targetDir / "lib", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

            // 3. 拷贝资源目录
            if (std::filesystem::exists("assets")) {
                std::filesystem::copy("assets", targetDir / "assets", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
            }

            LOG_INFO("Packaging", "Project packaged successfully!");
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("Packaging", "Packaging failed: {0}", e.what());
            return false;
        }
    }
};

} // namespace Prisma
