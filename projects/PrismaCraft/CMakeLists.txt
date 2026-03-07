cmake_minimum_required(VERSION 3.20)
project(PrismaCraft VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 尝试查找已安装的 SDK (独立模式)
find_package(PrismaEngine QUIET)

# 如果没有找到已安装的 SDK，检查是否可用构建目录中的 Engine (内部模式)
if(NOT PrismaEngine_FOUND AND TARGET Engine)
    # 内部构建模式：Engine 已在主构建中创建
    set(PRISMACRAFT_USE_ENGINE_FROM_BUILD ON)
    message(STATUS "PrismaCraft: Using Engine from build directory (internal mode)")
elseif(PrismaEngine_FOUND)
    # 独立模式：使用已安装的 SDK
    set(PRISMACRAFT_USE_ENGINE_FROM_BUILD OFF)
    message(STATUS "PrismaCraft: Using installed PrismaEngine SDK")
else()
    message(FATAL_ERROR "PrismaCraft requires PrismaEngine. Either build with SDK mode or install PrismaEngine SDK first.")
endif()

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
if(PRISMACRAFT_USE_ENGINE_FROM_BUILD)
    target_link_libraries(PrismaCraft PUBLIC Engine)
    message(STATUS "PrismaCraft linked to Engine (internal build)")
else()
    target_link_libraries(PrismaCraft PUBLIC PrismaEngine::Engine)
    message(STATUS "PrismaCraft linked to PrismaEngine::Engine (SDK)")
endif()

# 导出宏定义
target_compile_definitions(PrismaCraft PRIVATE PRISMACRAFT_EXPORTS)

message(STATUS "PrismaCraft configured successfully")
