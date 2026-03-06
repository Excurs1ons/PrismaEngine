cmake_minimum_required(VERSION 3.20)
project(PrismaCraft VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 查找 PrismaEngine SDK
# 提示: 用户可以通过 -DPrismaEngine_DIR=F:/repos/PrismaEngine/sdk/out/lib/cmake/PrismaEngine 传入
find_package(PrismaEngine REQUIRED)

# 2. 包含头文件
# 注意: 这里的路径反映了我们之前看到的套娃结构
set(PRISMACRAFT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/include
    ${CMAKE_CURRENT_SOURCE_DIR}/branches/native-cpp/src
)

# 3. 收集源文件 (目前主要是头文件，我们要为 .cpp 留出位置)
file(GLOB_RECURSE PRISMACRAFT_SOURCES
    "branches/native-cpp/src/*.h"
    "branches/native-cpp/src/*.cpp"
)

# 4. 创建库或可执行文件
# 暂时作为静态库，因为它看起来更像是一个逻辑层
add_library(PrismaCraft STATIC ${PRISMACRAFT_SOURCES})

target_include_directories(PrismaCraft PUBLIC ${PRISMACRAFT_INCLUDE_DIRS})

# 5. 链接引擎
target_link_libraries(PrismaCraft PRIVATE PrismaEngine::Engine)

# 定义导出宏 (模拟 PrismaCraft.h 里的需求)
target_compile_definitions(PrismaCraft PRIVATE PRISMACRAFT_EXPORTS)

message(STATUS "PrismaCraft configured to use PrismaEngine SDK")
