cmake_minimum_required(VERSION 3.20)
project(BasicTriangle VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── 查找 PrismaEngine SDK ──
# 使用预编译 SDK (从 GitHub Releases 下载):
#   cmake -B build -DPrismaEngine_DIR=/path/to/PrismaEngine-SDK-<version>-<platform>
#
# 使用仓库内 SDK (本地开发):
#   cmake -B build -DPrismaEngine_DIR=$(pwd)/../..
#
# 使用引擎构建产物 (源码构建):
#   cmake -B build -DPrismaEngine_DIR=$(pwd)/../../../build/<preset>
# ──────────────────────────

find_package(PrismaEngine REQUIRED)

# 创建可执行文件
add_executable(BasicTriangle
    src/main.cpp
)

# 链接 PrismaEngine
target_link_libraries(BasicTriangle PRIVATE PrismaEngine::Engine)

# 包含目录
target_include_directories(BasicTriangle PRIVATE
    ${PRISMAENGINE_INCLUDE_DIR}
)

set_target_properties(BasicTriangle PROPERTIES
    OUTPUT_NAME "BasicTriangle"
)

install(TARGETS BasicTriangle
    RUNTIME DESTINATION bin
)
