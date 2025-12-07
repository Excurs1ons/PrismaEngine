cmake_minimum_required(VERSION 3.31)
project(PRISMA VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts)")
    execute_process(
        COMMAND git submodule update --init --recursive --force
    )
    message("vcpkg submodule updated")
endif()

if(MSVC)
    # 使用 /utf-8 选项，它相当于同时设置了 /source-charset:utf-8 和 /execution-charset:utf-8
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

# vcpkg集成
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json")
    # 首先尝试项目内部的vcpkg
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file" FORCE)
    # 如果项目内没有vcpkg，则尝试使用环境变量VCPKG_ROOT
    elseif(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file" FORCE)
    else()
        message(FATAL_ERROR "Vcpkg toolchain file not found")
    endif()


    if(DEFINED CMAKE_TOOLCHAIN_FILE)
        set(VCPKG_MANIFEST_MODE ON)

        # 启用vcpkg manifest以解决构建时的警告
        set(VCPKG_ENABLE_MANIFEST ON)

        # 对于Visual Studio生成器，传递VcpkgEnableManifest参数给MSBuild
        if(CMAKE_GENERATOR MATCHES "Visual Studio")
            set(CMAKE_VS_GLOBAL_PROJECT_PROPERTIES "${CMAKE_CURRENT_SOURCE_DIR}/Directory.Build.props")
        endif()
    endif()
endif()

# 设置输出目录，使可执行文件、动态库和静态库分别放在不同的目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 添加子目录
add_subdirectory(src/core)
add_subdirectory(src/editor)
add_subdirectory(src/runtime)
add_subdirectory(src/game)

# 创建一个聚合目标，确保CLion能正确识别项目
add_custom_target(Prisma ALL
        DEPENDS Engine Editor Runtime Game
)