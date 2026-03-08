# EditorTargets.cmake
# 编辑器目标定义
# 此文件定义可独立构建的 Editor 目标

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

    # 编辑器源文件配置（这些文件实际在 src/editor/CMakeLists.txt 中定义）
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

    # 编辑器头文件配置（这些文件实际在 src/editor/CMakeLists.txt 中定义）
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

    # 编辑器资源文件
    set(PRISMA_EDITOR_RESOURCES
        "resources/editor"
        "resources/common"
    )

    # 安装编辑器资源 (仅在启用 install 时生效)
    if(PRISMA_ENABLE_INSTALL)
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
    endif()

    # 编辑器插件系统
    option(PRISMA_EDITOR_ENABLE_PLUGINS "启用编辑器插件系统" ON)
    if(PRISMA_EDITOR_ENABLE_PLUGINS)
        # 定义插件接口
        set(PRISMA_EDITOR_PLUGIN_API_VERSION 1)

        # 插件目录
        set(PRISMA_EDITOR_PLUGIN_DIR "plugins" CACHE STRING "编辑器插件目录")

        # 安装插件 API 头文件 (仅在启用 install 时生效)
        if(PRISMA_ENABLE_INSTALL)
            install(FILES
                src/editor/api/EditorPlugin.h
                src/editor/api/EditorPanel.h
                src/editor/api/EditorWindow.h
                DESTINATION include/PrismaEngine/editor/api
            )
        endif()
    endif()

else()
    message(STATUS "Editor: 构建已禁用")
endif()

# ========== Android 编辑器配置 ==========

if(ANDROID AND PRISMA_BUILD_EDITOR)
    message(WARNING "Android 平台不支持编辑器，自动禁用")
    set(PRISMA_BUILD_EDITOR OFF CACHE BOOL "构建编辑器" FORCE)
endif()
