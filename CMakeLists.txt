cmake_minimum_required(VERSION 3.31)
project(PRISMA VERSION 1.0.0 LANGUAGES CXX)

# 检查是否在 Visual Studio 中
if (CMAKE_GENERATOR MATCHES "Visual Studio")
    # 设置 Windows SDK 版本以确保使用最新版本
    set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "Windows SDK Version" FORCE)
    # 确保使用最新的 Windows SDK
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "latest" CACHE STRING "WindowsTargetPlatformVersion" FORCE)
endif()

# 启用 C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置默认的安装前缀
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Default install path" FORCE)
endif()

if(DEFINED CMAKE_TOOLCHAIN_FILE)
    set(VCPKG_MANIFEST_MODE ON)

    # 启用vcpkg manifest以解决构建时的警告
    set(VCPKG_ENABLE_MANIFEST ON)

    # 对于Visual Studio生成器，传递VcpkgEnableManifest参数给MSBuild
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        set(CMAKE_VS_GLOBAL_PROJECT_PROPERTIES "configs/Directory.Build.props")
    endif()
endif()


# 设置输出目录，使可执行文件、动态库和静态库分别放在不同的目录
# 对于多配置生成器，为每种配置设置不同的输出目录
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} UPPER_CONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${UPPER_CONFIG} ${CMAKE_BINARY_DIR}/bin/${CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${UPPER_CONFIG} ${CMAKE_BINARY_DIR}/lib/${CONFIG})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${UPPER_CONFIG} ${CMAKE_BINARY_DIR}/lib/${CONFIG})
endforeach()

# 单配置生成器的默认设置
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 添加子目录
add_subdirectory(src/core)
add_subdirectory(src/runtime)
add_subdirectory(src/game)
add_subdirectory(src/editor)

# 安装projects目录
install(DIRECTORY projects/
    DESTINATION projects
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN "build" EXCLUDE
    PATTERN ".gradle" EXCLUDE
)

# 安装resources目录
install(DIRECTORY resources/
    DESTINATION resources
    PATTERN "build" EXCLUDE
)

# 创建分发目标
# 更健壮的构建类型判断
if(CMAKE_BINARY_DIR MATCHES "/debug/" OR CMAKE_BINARY_DIR MATCHES "\\debug$")
    set(DIST_CONFIG "Debug")
elseif(CMAKE_BINARY_DIR MATCHES "/release/" OR CMAKE_BINARY_DIR MATCHES "\\release$")
    set(DIST_CONFIG "Release")
else()
    # 对于多配置生成器(如Visual Studio)，使用默认配置
    set(DIST_CONFIG "Release")
    message(WARNING "无法从构建路径确定配置类型，使用默认Release配置")
endif()

add_custom_target(dist
    COMMAND ${CMAKE_COMMAND} -E echo "使用配置: ${DIST_CONFIG} 创建分发包"
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config ${DIST_CONFIG} --prefix ${CMAKE_BINARY_DIR}/install
    COMMAND ${CMAKE_COMMAND} -E tar "czf" "${CMAKE_BINARY_DIR}/PrismaEngine-${PROJECT_VERSION}.tar.gz" 
        --format=gnutar 
        "${CMAKE_BINARY_DIR}/install"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating distribution package"
    VERBATIM
)

add_custom_target(dist-win
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config ${DIST_CONFIG} --prefix ${CMAKE_BINARY_DIR}/install
    COMMAND ${CMAKE_COMMAND} -E tar "cf" "${CMAKE_BINARY_DIR}/PrismaEngine-${PROJECT_VERSION}.zip" 
        --format=zip
        "${CMAKE_BINARY_DIR}/install"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating Windows distribution package"
    VERBATIM
)

# 创建一个聚合目标，确保CLion能正确识别项目
add_custom_target(Prisma ALL
        DEPENDS Engine Editor Runtime Game
)

# CPack打包配置
set(CPACK_PACKAGE_NAME "PrismaEngine")
set(CPACK_PACKAGE_VENDOR "Prisma")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Prisma Game Engine")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "PrismaEngine")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Windows特定配置
if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "Prisma Engine")
    set(CPACK_NSIS_PACKAGE_NAME "Prisma Engine")
    set(CPACK_NSIS_CONTACT "contact@example.com")
    set(CPACK_NSIS_MODIFY_PATH ON)
endif()

# 其他平台配置
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ;DEB")
endif()

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
endif()

# 安装配置文件
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PrismaEngineConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/PrismaEngineConfig.cmake"
    INSTALL_DESTINATION lib/cmake/PrismaEngine
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/PrismaEngineConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES 
    "${CMAKE_CURRENT_BINARY_DIR}/PrismaEngineConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/PrismaEngineConfigVersion.cmake"
    DESTINATION lib/cmake/PrismaEngine
)

install(EXPORT PrismaEngineTargets
    FILE PrismaEngineTargets.cmake
    NAMESPACE PrismaEngine::
    DESTINATION lib/cmake/PrismaEngine
)

include(CPack)