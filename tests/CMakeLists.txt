cmake_minimum_required(VERSION 3.20)

project(PrismaEngineTests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖包
find_package(DirectXMath REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 收集测试源文件（排除TestMain.cpp，它会被单独添加）
file(GLOB TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*Test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# 创建测试可执行文件
add_executable(PrismaEngineTests
    TestMain.cpp
    ${TEST_SOURCES}
)

# 链接库
target_link_libraries(PrismaEngineTests
    PRIVATE
    ${DirectXMath_LIBRARIES}
)

# 在Windows上设置子系统和入口点
if(WIN32)
    set_target_properties(PrismaEngineTests PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# 添加编译定义
target_compile_definitions(PrismaEngineTests
    PRIVATE
    TESTING=1
)