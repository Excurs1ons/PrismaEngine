# PackagingConfig.cmake
# CPack 打包配置 — 构建 NSIS (Windows) / TGZ+DEB+RPM (Linux) 安装包
# 使用方式: cmake --build build --target package

# ========== CPack 通用配置 ==========

set(CPACK_PACKAGE_NAME "PrismaEngine")
set(CPACK_PACKAGE_VENDOR "Prisma")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Prisma Game Engine — cross-platform 3D engine built with modern C++20")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "PrismaEngine")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# 打包内容来自 cmake --install, 由 InstallConfig.cmake 定义安装规则
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};PRISMA;ALL;/")

# ========== Windows: NSIS 安装包 ==========

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "Prisma Engine")
    set(CPACK_NSIS_PACKAGE_NAME "Prisma Engine")
    set(CPACK_NSIS_CONTACT "contact@example.com")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    set(CPACK_NSIS_MENU_LINKS
        "bin/PrismaEditor.exe" "Prisma Engine Editor"
        "bin/PrismaLauncher.exe" "Prisma Engine Launcher"
    )
endif()

# ========== Linux: TGZ / DEB / RPM ==========

if(PRISMA_PLATFORM_LINUX)
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6, libvulkan1")
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
endif()

# ========== 启用 CPack ==========

include(CPack)
