# PackagingConfig.cmake
# CPack 打包配置和自定义分发目标

# ========== 构建配置检测 ==========

# 检测当前构建配置，用于分发目标
if(CMAKE_BINARY_DIR MATCHES "/debug/" OR CMAKE_BINARY_DIR MATCHES "\\debug$")
    set(DIST_CONFIG "Debug")
elseif(CMAKE_BINARY_DIR MATCHES "/release/" OR CMAKE_BINARY_DIR MATCHES "\\release$")
    set(DIST_CONFIG "Release")
else()
    # 对于多配置生成器 (如 Visual Studio)，使用默认配置
    set(DIST_CONFIG "Release")
    message(WARNING "无法从构建路径确定配置类型，使用默认 Release 配置")
endif()

# ========== CPack 配置 ==========

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

# Windows 特定配置
if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "Prisma Engine")
    set(CPACK_NSIS_PACKAGE_NAME "Prisma Engine")
    set(CPACK_NSIS_CONTACT "contact@example.com")
    set(CPACK_NSIS_MODIFY_PATH ON)
endif()

# Linux 特定配置
if(PRISMA_PLATFORM_LINUX)
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgl1-mesa-glx, libopenal1")
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
endif()

# ========== 自定义分发目标 ==========

# tar.gz 分发目标 (Linux/macOS/通用)
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

# zip 分发目标 (Windows)
add_custom_target(dist-win
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config ${DIST_CONFIG} --prefix ${CMAKE_BINARY_DIR}/install
    COMMAND ${CMAKE_COMMAND} -E tar "cf" "${CMAKE_BINARY_DIR}/PrismaEngine-${PROJECT_VERSION}.zip"
        --format=zip
        "${CMAKE_BINARY_DIR}/install"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating Windows distribution package"
    VERBATIM
)

# 包含 CPack
# 注意: 如果需要使用 CPack，取消下面的注释
# include(CPack)
