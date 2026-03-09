# RuntimeTargets.cmake
# 运行时可执行文件目标定义
# 此文件定义可独立构建的 Runtime 可执行文件目标

# ========== 构建选项 ==========

option(PRISMA_BUILD_RUNTIME "构建运行时" ON)
option(PRISMA_RUNTIME_DYNAMIC_LOAD "运行时动态加载游戏模块" OFF)

# ========== Runtime 目标定义 ==========

if(PRISMA_BUILD_RUNTIME)
    message(STATUS "Runtime: 构建已启用")

    # 定义运行时配置
    set(PRISMA_RUNTIME_CONFIG "Release" CACHE STRING "运行时配置")
    set_property(CACHE PRISMA_RUNTIME_CONFIG PROPERTY STRINGS "Debug" "Release")

    # 运行时依赖
    set(PRISMA_RUNTIME_DEPENDENCIES
        "Engine"
    )

    # 如果不是动态加载模式，静态链接 Game
    if(NOT PRISMA_RUNTIME_DYNAMIC_LOAD)
        list(APPEND PRISMA_RUNTIME_DEPENDENCIES "Game")
        message(STATUS "Runtime: 静态链接 Game 模块")
    else()
        message(STATUS "Runtime: 动态加载 Game 模块")
    endif()

    # 创建运行时目标（实际的创建在 src/runtime/CMakeLists.txt）
    add_custom_target(RuntimeBuild
        DEPENDS Runtime
        COMMENT "构建 PrismaEngine 运行时"
    )

    # 运行时源文件配置
    set(PRISMA_RUNTIME_SOURCES
        "src/runtime/windows/WindowsRuntime.cpp"
        "src/runtime/linux/LinuxRuntime.cpp"
        "src/runtime/android/AndroidRuntime.cpp"
        "src/runtime/web/WebRuntime.cpp"
    )

    # 运行时资源文件
    set(PRISMA_RUNTIME_RESOURCES
        "resources/common"
        "resources/runtime"
    )

else()
    message(STATUS "Runtime: 构建已禁用")
endif()

# ========== 运行时平台特定配置 ==========

# Windows 运行时配置
if(WIN32 AND PRISMA_BUILD_RUNTIME)
    # Windows 特定设置
    set(PRISMA_RUNTIME_WINDOWS_ICON
        "resources/windows/icons/Runtime.ico"
    )

    # Windows 运行时清单
    set(PRISMA_RUNTIME_WINDOWS_MANIFEST
        "resources/windows/app.manifest"
    )

    # Windows 子系统设置
    set(PRISMA_RUNTIME_WINDOWS_SUBSYSTEM "WINDOWS" CACHE STRING "Windows 子系统")
    set_property(CACHE PRISMA_RUNTIME_WINDOWS_SUBSYSTEM PROPERTY STRINGS "CONSOLE" "WINDOWS")

    if(PRISMA_RUNTIME_WINDOWS_SUBSYSTEM STREQUAL "WINDOWS")
        # target_link_options 将在 src/runtime/CMakeLists.txt 中设置
        set(PRISMA_RUNTIME_WINDOWS_SUBSYSTEM_CONFIG "WINDOWS")
    else()
        set(PRISMA_RUNTIME_WINDOWS_SUBSYSTEM_CONFIG "CONSOLE")
    endif()

    # DPI 感知 - 将在 src/runtime/CMakeLists.txt 中设置
    set(PRISMA_RUNTIME_DPI_AWARENESS ON)
endif()

# Linux 运行时配置
if(LINUX AND PRISMA_BUILD_RUNTIME)
    # Linux 桌面文件 (仅在启用 install 时生效)
    if(PRISMA_ENABLE_INSTALL)
        install(FILES
            resources/linux/prismaruntime.desktop
            DESTINATION share/applications
        )

        # Linux 图标
        install(FILES
            resources/linux/icons/prismaruntime.png
            DESTINATION share/icons/hicolor/256x256/apps
        )
    endif()

    # Linux AppImage 配置（可选）
    option(PRISMA_RUNTIME_BUILD_APPIMAGE "构建 Linux AppImage" OFF)
    if(PRISMA_RUNTIME_BUILD_APPIMAGE)
        find_program(LINUXDEPLOY_EXECUTABLE linuxdeploy)
        if(LINUXDEPLOY_EXECUTABLE)
            add_custom_target(appimage
                COMMAND ${LINUXDEPLOY_EXECUTABLE}
                    --appdir=AppDir
                    --executable=$<TARGET_FILE:Runtime>
                    --icon-file=resources/linux/icons/prismaruntime.png
                    --desktop-file=resources/linux/prismaruntime.desktop
                    --output=appimage
                COMMENT "构建 Linux AppImage"
            )
        else()
            message(WARNING "linuxdeploy 未找到，跳过 AppImage 构建")
        endif()
    endif()
endif()

# Android 运行时配置
if(ANDROID AND PRISMA_BUILD_RUNTIME)
    # Android 运行时配置在 projects/android/PrismaAndroid 中
    message(STATUS "Android 运行时配置在 Gradle 构建脚本中")

    # Android 库配置
    set(PRISMA_ANDROID_APP_NAME "PrismaRuntime" CACHE STRING "Android 应用名称")
    set(PRISMA_ANDROID_PACKAGE_NAME "com.prismaengine.runtime" CACHE STRING "Android 包名")

    # Android 权限
    set(PRISMA_ANDROID_PERMISSIONS
        "android.permission.INTERNET"
        "android.permission.WRITE_EXTERNAL_STORAGE"
    )

    # Android 特定编译定义（仅当 Runtime 目标存在时）
    if(TARGET Runtime)
        target_compile_definitions(Runtime PRIVATE
            PRISMA_ANDROID_PLATFORM=1
        )
    endif()
endif()

# WebAssembly 运行时配置
if(EMSCRIPTEN AND PRISMA_BUILD_RUNTIME)
    set(PRISMA_RUNTIME_DYNAMIC_LOAD OFF CACHE BOOL "Web runtime disables dynamic loading" FORCE)
    message(STATUS "WebAssembly 运行时: 静态链接 Game 模块")
endif()

# ========== 运行时测试支持 ==========

option(PRISMA_RUNTIME_BUILD_TESTS "构建运行时测试" OFF)
if(PRISMA_RUNTIME_BUILD_TESTS)
    enable_testing()

    # 添加运行时测试
    add_subdirectory(tests/runtime)

    # 测试目标
    add_custom_target(test-runtime
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS Runtime
        COMMENT "运行运行时测试"
    )
endif()
