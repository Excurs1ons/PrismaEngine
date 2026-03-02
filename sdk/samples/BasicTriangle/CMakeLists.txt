cmake_minimum_required(VERSION 3.20)
project(BasicTriangle VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 查找 PrismaEngine
# 如果从 SDK 构建：
# set(PrismaEngine_DIR "/path/to/sdk")
# 如果从源码构建：
# set(PrismaEngine_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

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

# 设置输出名称
set_target_properties(BasicTriangle PROPERTIES
    OUTPUT_NAME "BasicTriangle"
)

# 安装规则
install(TARGETS BasicTriangle
    RUNTIME DESTINATION bin
)
