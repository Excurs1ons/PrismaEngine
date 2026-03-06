cmake_minimum_required(VERSION 3.20)
project(BlockGame VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 查找 PrismaEngine SDK
find_package(PrismaEngine REQUIRED)

# 2. 查找 PrismaCraft 逻辑库
# 假设 PrismaCraft 已经作为引擎项目的一部分被构建或安装
# 在开发环境下，我们可以直接引用项目目录
find_package(PrismaCraft REQUIRED)

# 3. 收集源文件
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# 4. 创建可执行文件
add_executable(BlockGame ${SOURCES})

# 5. 链接引擎和逻辑库
target_link_libraries(BlockGame PRIVATE PrismaEngine::Engine PrismaCraft::PrismaCraft)

# 拷贝资源
add_custom_command(TARGET BlockGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:BlockGame>/assets
    COMMENT "Copying assets to output directory"
)
