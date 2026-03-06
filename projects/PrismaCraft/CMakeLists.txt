cmake_minimum_required(VERSION 3.20)
project(PrismaCraft VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 查找 PrismaEngine SDK
find_package(PrismaEngine REQUIRED)

# 2. 包含路径
set(PRISMACRAFT_CORE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PRISMACRAFT_LOGIC_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/src")

# 3. 收集源文件
file(GLOB_RECURSE PRISMACRAFT_SOURCES
    "src/*.cpp"
    "src/*.h"
    "include/*.h"
)

# 4. 创建 PrismaCraft 库
add_library(PrismaCraft STATIC ${PRISMACRAFT_SOURCES})
add_library(PrismaCraft::PrismaCraft ALIAS PrismaCraft)

target_include_directories(PrismaCraft PUBLIC 
    $<BUILD_INTERFACE:${PRISMACRAFT_CORE_INCLUDE}>
    $<BUILD_INTERFACE:${PRISMACRAFT_LOGIC_INCLUDE}>
)

# 5. 链接引擎
target_link_libraries(PrismaCraft PRIVATE PrismaEngine::Engine)

# 导出宏定义
target_compile_definitions(PrismaCraft PRIVATE PRISMACRAFT_EXPORTS)

# 显示配置成功信息
message(STATUS "PrismaCraft (Integrated) configured using PrismaEngine SDK")
