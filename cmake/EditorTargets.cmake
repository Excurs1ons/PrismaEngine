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

else()
    message(STATUS "Editor: 构建已禁用")
endif()

# ========== Android 编辑器配置 ==========

if(ANDROID AND PRISMA_BUILD_EDITOR)
    message(WARNING "Android 平台不支持编辑器，自动禁用")
    set(PRISMA_BUILD_EDITOR OFF CACHE BOOL "构建编辑器" FORCE)
endif()
