# InstallConfig.cmake
# 安装规则配置

# ========== Install 功能控制 ==========

# 添加选项控制是否启用 install 功能
option(PRISMA_ENABLE_INSTALL "启用 CMake install 功能 (用于 SDK 打包)" OFF)

# 如果禁用 install，直接返回
if(NOT PRISMA_ENABLE_INSTALL)
    return()
endif()

# ========== Engine 库安装 ==========

if(PRISMA_BUILD_ENGINE AND TARGET Engine)
    # 安装引擎库
    install(TARGETS Engine
        EXPORT PrismaEngineTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
    )

    # 安装公共头文件
    install(DIRECTORY src/engine/
        DESTINATION include/PrismaEngine
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*/adapters/*" EXCLUDE
        PATTERN "*/drivers/*" EXCLUDE
        PATTERN "*/platform/*" EXCLUDE
        PATTERN "*.cpp" EXCLUDE
    )

    # 安装 ECS 核心头文件
    install(DIRECTORY src/engine/core/
        DESTINATION include/PrismaEngine/core
        FILES_MATCHING PATTERN "*.h"
    )

    # 安装物理系统头文件
    install(FILES
        src/engine/physics/CollisionSystem.h
        src/engine/physics/FrustumCulling.h
        DESTINATION include/PrismaEngine/physics
    )

    # 安装渲染接口
    install(DIRECTORY src/engine/graphic/interfaces/
        DESTINATION include/PrismaEngine/graphic/interfaces
        FILES_MATCHING PATTERN "*.h"
    )

    # 安装纹理系统头文件
    install(FILES
        src/engine/graphic/TextureAtlas.h
        src/engine/graphic/FrustumCulling.h
        DESTINATION include/PrismaEngine/graphic
    )

    # 安装输入系统头文件
    install(FILES
        src/engine/input/InputManager.h
        src/engine/input/InputDevice.h
        src/engine/input/EnhancedInputManager.h
        DESTINATION include/PrismaEngine/input
    )

    # 安装音频系统头文件
    install(FILES
        src/engine/audio/AudioManager.h
        src/engine/audio/IAudioDevice.h
        src/engine/audio/AudioAPI.h
        DESTINATION include/PrismaEngine/audio
    )
endif()

# ========== Runtime 可执行文件安装 ==========

if(PRISMA_BUILD_RUNTIME AND TARGET Runtime)
    # 安装运行时可执行文件
    install(TARGETS Runtime
        RUNTIME DESTINATION bin
    )
endif()

# ========== 目录安装 ==========

# 安装 projects 目录
install(DIRECTORY projects/
    DESTINATION projects
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN "build" EXCLUDE
    PATTERN ".gradle" EXCLUDE
    PATTERN "CMakeFiles" EXCLUDE
    PATTERN "cmake_install.cmake" EXCLUDE
    PATTERN "Makefile" EXCLUDE
)

# 安装 assets 目录
install(DIRECTORY assets/
    DESTINATION assets
    PATTERN "build" EXCLUDE
    PATTERN ".DS_Store" EXCLUDE
)

# ========== 可选: 安装 resources 目录 ==========
# 取消下面的注释以启用 resources 目录安装
# install(DIRECTORY resources/
#     DESTINATION resources
#     PATTERN "build" EXCLUDE
#     PATTERN ".DS_Store" EXCLUDE
# )

# ========== CMake 包配置文件 ==========
# 用于外部项目使用引擎

include(CMakePackageConfigHelpers)

# 配置包文件
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PrismaEngineConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/PrismaEngineConfig.cmake"
    INSTALL_DESTINATION lib/cmake/PrismaEngine
)

# 写入版本文件
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/PrismaEngineConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 安装配置文件
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/PrismaEngineConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/PrismaEngineConfigVersion.cmake"
    DESTINATION lib/cmake/PrismaEngine
)

# 安装导出目标
install(EXPORT PrismaEngineTargets
    FILE PrismaEngineTargets.cmake
    NAMESPACE PrismaEngine::
    DESTINATION lib/cmake/PrismaEngine
)

