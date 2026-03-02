# InstallConfig.cmake
# 安装规则配置

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

