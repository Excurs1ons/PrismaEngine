# EditorTargets.cmake
# 编辑器目标定义
# 此文件定义可独立构建的 Editor 目标

# ========== 构建选项 ==========

option(PRISMA_BUILD_EDITOR "构建编辑器" ON)

# ========== Editor 目标定义 ==========

if(PRISMA_BUILD_EDITOR)
    message(STATUS "Editor: 构建已启用")

    # 编辑器依赖检查
    find_package(imgui QUIET)
    if(NOT imgui_FOUND AND NOT TARGET imgui AND NOT TARGET imgui::imgui)
        message(FATAL_ERROR "Editor 需要 ImGui，但未找到。请启用 PRISMA_ENABLE_IMGUI_DEBUG")
    endif()

    # 定义编辑器组件
    set(PRISMA_EDITOR_COMPONENTS
        "SceneEditor"       # 场景编辑器
        "AssetBrowser"      # 资源浏览器
        "PropertyEditor"    # 属性编辑器
        "Console"           # 控制台
        "Profiler"          # 性能分析器
        "EntityInspector"   # 实体检查器
        "ComponentEditor"   # 组件编辑器
        "MaterialEditor"    # 材质编辑器
        "ShaderEditor"      # 着色器编辑器
        "ParticleSystemEditor"  # 粒子系统编辑器
        "AnimationEditor"   # 动画编辑器
    )

    message(STATUS "Editor 组件: ${PRISMA_EDITOR_COMPONENTS}")

    # 编辑器依赖
    set(PRISMA_EDITOR_DEPENDENCIES
        "Engine"
        "imgui::imgui"
    )

    # 编辑器源文件配置
    set(PRISMA_EDITOR_SOURCES
        "src/editor/editor.cpp"
        "src/editor/panels/ScenePanel.cpp"
        "src/editor/panels/AssetBrowserPanel.cpp"
        "src/editor/panels/PropertyPanel.cpp"
        "src/editor/panels/ConsolePanel.cpp"
        "src/editor/panels/ProfilerPanel.cpp"
        "src/editor/gizmos/Gizmo.cpp"
        "src/editor/gizmos/TransformGizmo.cpp"
        "src/editor/windows/MainWindow.cpp"
        "src/editor/windows/EditorLayout.cpp"
    )

    # 编辑器头文件配置
    set(PRISMA_EDITOR_HEADERS
        "src/editor/editor.h"
        "src/editor/panels/ScenePanel.h"
        "src/editor/panels/AssetBrowserPanel.h"
        "src/editor/panels/PropertyPanel.h"
        "src/editor/panels/ConsolePanel.h"
        "src/editor/panels/ProfilerPanel.h"
        "src/editor/gizmos/Gizmo.h"
        "src/editor/gizmos/TransformGizmo.h"
        "src/editor/windows/MainWindow.h"
        "src/editor/windows/EditorLayout.h"
    )

    # 创建编辑器目标（实际的创建在 src/editor/CMakeLists.txt）
    add_custom_target(EditorBuild
        DEPENDS Editor
        COMMENT "构建 PrismaEngine 编辑器"
    )

    # 编辑器资源文件
    set(PRISMA_EDITOR_RESOURCES
        "resources/editor"
        "resources/common
    )

    # 编辑器安装配置
    install(TARGETS Editor
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )

    # 安装编辑器资源
    install(DIRECTORY ${PRISMA_EDITOR_RESOURCES}
        DESTINATION .
        USE_SOURCE_PERMISSIONS
        PATTERN ".git" EXCLUDE
    )

    # 安装编辑器着色器
    install(DIRECTORY resources/editor/shaders/
        DESTINATION shaders/editor
        FILES_MATCHING
        PATTERN "*.hlsl"
        PATTERN "*.glsl"
    )

    # 编辑器插件系统
    option(PRISMA_EDITOR_ENABLE_PLUGINS "启用编辑器插件系统" ON)
    if(PRISMA_EDITOR_ENABLE_PLUGINS)
        # 定义插件接口
        set(PRISMA_EDITOR_PLUGIN_API_VERSION 1)

        # 插件目录
        set(PRISMA_EDITOR_PLUGIN_DIR "plugins" CACHE STRING "编辑器插件目录")

        # 安装插件 API 头文件
        install(FILES
            src/editor/api/EditorPlugin.h
            src/editor/api/EditorPanel.h
            src/editor/api/EditorWindow.h
            DESTINATION include/PrismaEngine/editor/api
        )
    endif()

else()
    message(STATUS "Editor: 构建已禁用")
endif()

# ========== 编辑器平台特定配置 ==========

# Windows 编辑器配置
if(WIN32 AND PRISMA_BUILD_EDITOR)
    # Windows 编辑器图标
    set(PRISMA_EDITOR_WINDOWS_ICON
        "resources/windows/icons/Editor.ico"
    )

    # Windows 编辑器清单
    set(PRISMA_EDITOR_WINDOWS_MANIFEST
        "resources/windows/app.manifest"
    )

    # Windows 子系统设置
    set(PRISMA_EDITOR_WINDOWS_SUBSYSTEM "WINDOWS" CACHE STRING "Windows 子系统")
    set_property(CACHE PRISMA_EDITOR_WINDOWS_SUBSYSTEM PROPERTY STRINGS "CONSOLE" "WINDOWS")

    if(PRISMA_EDITOR_WINDOWS_SUBSYSTEM STREQUAL "WINDOWS")
        target_link_options(Editor PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    endif()

    # DPI 感知
    target_compile_definitions(Editor PRIVATE
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2=1
    )

    # Windows 崩溃转储
    option(PRISMA_EDITOR_ENABLE_MINIDUMP "启用编辑器崩溃转储" ON)
    if(PRISMA_EDITOR_ENABLE_MINIDUMP)
        target_compile_definitions(Editor PRIVATE
            PRISMA_ENABLE_MINIDUMP=1
        )
        target_link_libraries(Editor PRIVATE
            dbghelp.lib
        )
    endif()
endif()

# Linux 编辑器配置
if(LINUX AND PRISMA_BUILD_EDITOR)
    # Linux 桌面文件
    install(FILES
        resources/linux/prismaeditor.desktop
        DESTINATION share/applications
    )

    # Linux 图标
    install(FILES
        resources/linux/icons/prismaeditor.png
        DESTINATION share/icons/hicolor/256x256/apps
    )

    # Linux AppImage 配置（可选）
    option(PRISMA_EDITOR_BUILD_APPIMAGE "构建 Linux AppImage" OFF)
    if(PRISMA_EDITOR_BUILD_APPIMAGE)
        find_program(LINUXDEPLOY_EXECUTABLE linuxdeploy)
        if(LINUXDEPLOY_EXECUTABLE)
            find_program(LINUXDEPLOY_QT_EXECUTABLE linuxdeploy-plugin-qt)
            if(LINUXDEPLOY_QT_EXECUTABLE)
                add_custom_target(editor-appimage
                    COMMAND ${LINUXDEPLOY_EXECUTABLE}
                        --appdir=AppDir
                        --executable=$<TARGET_FILE:Editor>
                        --icon-file=resources/linux/icons/prismaeditor.png
                        --desktop-file=resources/linux/prismaeditor.desktop
                        --plugin=qt
                        --output=appimage
                    COMMENT "构建编辑器 Linux AppImage"
                )
            else()
                message(WARNING "linuxdeploy-plugin-qt 未找到，跳过 AppImage 构建")
            endif()
        else()
            message(WARNING "linuxdeploy 未找到，跳过 AppImage 构建")
        endif()
    endif()
endif()

# Android 编辑器配置
if(ANDROID AND PRISMA_BUILD_EDITOR)
    message(WARNING "Android 平台不支持编辑器，自动禁用")
    set(PRISMA_BUILD_EDITOR OFF CACHE BOOL "构建编辑器" FORCE)
endif()

# ========== 编辑器测试支持 ==========

option(PRISMA_EDITOR_BUILD_TESTS "构建编辑器测试" OFF)
if(PRISMA_EDITOR_BUILD_TESTS)
    enable_testing()

    # 添加编辑器测试
    add_subdirectory(tests/editor)

    # 测试目标
    add_custom_target(test-editor
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS Editor
        COMMENT "运行编辑器测试"
    )
endif()

# ========== 编辑器文档生成 ==========

option(PRISMA_EDITOR_BUILD_DOCS "构建编辑器文档" OFF)
if(PRISMA_EDITOR_BUILD_DOCS)
    find_program(DOXYGEN_EXECUTABLE doxygen)
    if(DOXYGEN_EXECUTABLE)
        add_custom_target(editor-docs
            COMMAND ${DOXYGEN_EXECUTABLE} docs/editor/Doxyfile
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "生成编辑器文档"
        )
    else()
        message(WARNING "Doxygen 未找到，跳过文档生成")
    endif()
endif()
