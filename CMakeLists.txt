# PrismaEngine 主 CMake 配置文件
# 此文件定义四个独立可构建目标：Engine、Runtime、Editor、SDK
#
# 支持的构建目标：
#   - Engine:      引擎核心库 (静态/动态库)
#   - Runtime:     运行时可执行文件
#   - Editor:      编辑器可执行文件 (可选)
#   - SDK:         SDK 打包目标
#
# 使用方法：
#   # 构建所有目标
#   cmake --build build
#
#   # 单独构建 Engine
#   cmake --build build --target Engine
#
#   # 打包 SDK
#   cmake --build build --target sdk-package

cmake_minimum_required(VERSION 3.31)

set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES "")

# ========== 项目声明 ==========

project(PRISMA
    VERSION 1.0.0
    DESCRIPTION "PrismaEngine - 现代跨平台游戏引擎"
    LANGUAGES CXX
)

# ========== 注释: 代理配置 ==========
# 本地开发时如果需要代理，请通过环境变量 HTTP_PROXY 和 HTTPS_PROXY 设置
# 不要在 CMakeLists.txt 中硬编码代理地址，这会导致 CI 环境构建失败
#set(ENV{HTTP_PROXY} "http://127.0.0.1:7897")
#set(ENV{HTTPS_PROXY} "http://127.0.0.1:7897")

# ========== 构建选项 ==========
option(PRISMA_BUILD_ENGINE "构建引擎核心库" ON)
option(PRISMA_BUILD_EDITOR "构建编辑器" ON)
option(PRISMA_BUILD_RUNTIME "构建运行时" ON)
option(PRISMA_BUILD_SDK "构建 SDK" OFF)
option(PRISMA_BUILD_TESTING "构建测试" OFF)

# ========== 包含配置模块 ==========

# 工具函数
include(cmake/Utils.cmake)

# 编译器和编译选项配置
include(cmake/CompilerOptions.cmake)

# 平台检测和配置
include(cmake/PlatformConfig.cmake)

# 输出目录配置
include(cmake/OutputDirectories.cmake)

# 设备选项配置 (需要在 FetchContent 之前)
include(cmake/DeviceOptions.cmake)

# ========== 依赖管理配置 ==========

option(PRISMA_USE_FETCHCONTENT "使用 CMake FetchContent 管理依赖" ON)


include(cmake/FetchThirdPartyDeps.cmake)


# ========== 包含目标配置模块 ==========

message(STATUS "")
message(STATUS "=== Loading Target Configuration ===")

# 加载引擎目标配置
include(cmake/EngineTargets.cmake)

# 加载运行时目标配置
include(cmake/RuntimeTargets.cmake)

# 加载编辑器目标配置 (如果启用)
if(PRISMA_BUILD_EDITOR)
    include(cmake/EditorTargets.cmake)
endif()

# 加载 SDK 配置 (如果启用)
if(PRISMA_BUILD_SDK)
    include(cmake/SDKConfig.cmake)
endif()

message(STATUS "====================================")
message(STATUS "")

# ========== 添加子模块 ==========

message(STATUS "")
message(STATUS "=== Adding Subdirectories ===")

# 核心引擎模块 (如果启用)
if(PRISMA_BUILD_ENGINE)
    add_subdirectory(src/engine)
    message(STATUS "已添加引擎模块")
else()
    message(STATUS "跳过引擎模块 (PRISMA_BUILD_ENGINE=OFF)")
endif()

# 编辑器模块 (如果启用)
if(PRISMA_BUILD_EDITOR)
    add_subdirectory(src/editor)
    message(STATUS "已添加编辑器模块")
    # 应用编辑器目标后配置（必须在 add_subdirectory 之后）
    include(cmake/EditorPostTargets.cmake)
else()
    # Game 模块 (Editor 模式下不需要 Game)
    if(EXISTS ${CMAKE_SOURCE_DIR}/src/game/CMakeLists.txt)
        add_subdirectory(src/game)
        message(STATUS "已添加 Game 模块")
    endif()
    
    # 外部项目模块 (PrismaCraft 等) - 在 SDK 模式下构建
    if(EXISTS ${CMAKE_SOURCE_DIR}/projects/CMakeLists.txt AND PRISMA_BUILD_SDK)
        # PrismaCraft 使用内部构建方式：直接从构建目录链接，不需要安装 SDK
        add_subdirectory(projects)
        message(STATUS "已添加 Projects 模块 (SDK 模式 - 内部构建)")
    endif()
endif()

# 运行时模块 (最后添加，以便能检测到 Game/Editor 目标)
if(PRISMA_BUILD_RUNTIME)
    add_subdirectory(src/runtime)
    message(STATUS "已添加运行时模块")
else()
    message(STATUS "跳过运行时模块 (PRISMA_BUILD_RUNTIME=OFF)")
endif()

message(STATUS "==============================")
message(STATUS "")

# ========== 安装配置 ==========

include(cmake/InstallConfig.cmake)

# ========== 项目目标和资源复制 ==========

# 此模块必须在所有子目录添加后调用，因为它依赖于 Engine/Editor/Runtime/Game 目标
include(cmake/ProjectTargets.cmake)

# ========== 打包配置 ==========

# 只在构建 SDK 时启用打包
if(PRISMA_BUILD_SDK)
    include(cmake/PackagingConfig.cmake)
endif()

# ========== 总结构建配置 ==========

message(STATUS "")
message(STATUS "=== Build Configuration Summary ===")
message(STATUS "  PrismaEngine Version: ${PROJECT_VERSION}")
message(STATUS "  Build Engine:    ${PRISMA_BUILD_ENGINE}")
message(STATUS "  Build Runtime:   ${PRISMA_BUILD_RUNTIME}")
message(STATUS "  Build Editor:    ${PRISMA_BUILD_EDITOR}")
message(STATUS "  Build SDK:       ${PRISMA_BUILD_SDK}")
message(STATUS "  Shared Library:  ${PRISMA_BUILD_SHARED_LIBS}")
message(STATUS "")
message(STATUS "  Render Backend: Vulkan")
message(STATUS "  Audio Backend: SDL3")
message(STATUS "")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "=====================================")
message(STATUS "")
