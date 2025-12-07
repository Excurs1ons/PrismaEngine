cmake_minimum_required(VERSION 3.31)
project(PRISMA VERSION 1.0.0 LANGUAGES CXX)

# 检查是否在 Visual Studio 中
if (CMAKE_GENERATOR MATCHES "Visual Studio")
    # 设置 Windows SDK 版本以确保使用最新版本
    set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "Windows SDK Version" FORCE)
    # 确保使用最新的 Windows SDK
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "latest" CACHE STRING "WindowsTargetPlatformVersion" FORCE)
endif()

# 启用 C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED CMAKE_TOOLCHAIN_FILE)
    set(VCPKG_MANIFEST_MODE ON)

    # 启用vcpkg manifest以解决构建时的警告
    set(VCPKG_ENABLE_MANIFEST ON)

    # 对于Visual Studio生成器，传递VcpkgEnableManifest参数给MSBuild
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        set(CMAKE_VS_GLOBAL_PROJECT_PROPERTIES "configs/Directory.Build.props")
    endif()
endif()


# 设置输出目录，使可执行文件、动态库和静态库分别放在不同的目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 添加子目录
add_subdirectory(src/core)
add_subdirectory(src/runtime)
add_subdirectory(src/game)
add_subdirectory(src/editor)

# 创建一个聚合目标，确保CLion能正确识别项目
add_custom_target(Prisma ALL
        DEPENDS Engine Editor Runtime Game
)