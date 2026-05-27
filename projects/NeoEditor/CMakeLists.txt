# ============================================================
# NeoEditor — PrismaEngine Editor
#   - EXE:  WinUI3 Desktop Application (Windows-only)
#   - DLL:  NeoEditor_plugin (Cross-platform, Launcher-loadable)
# ============================================================

cmake_minimum_required(VERSION 3.20)

project(NeoEditor
    VERSION 1.0.0
    DESCRIPTION "NeoEditor - PrismaEngine Editor"
    LANGUAGES CXX
)

# ========== 编译选项 ==========

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========== 插件源码 ==========

set(NEOEDITOR_PLUGIN_SOURCES
    NeoEditor_plugin/CreateApplication.cpp
    NeoEditor_plugin/NeoEditorPlugin.cpp
)

# ========== 共享编译选项宏 ==========

macro(setup_target_defaults target)
    target_link_libraries(${target} PRIVATE Engine)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4 /WX- /utf-8
            $<$<CONFIG:Release>:/O2 /Oi /Gy /Zi>
            $<$<CONFIG:MinSizeRel>:/O1 /Oi /Gy>
            $<$<CONFIG:RelWithDebInfo>:/O2 /Oi /Gy>
        )
        target_compile_definitions(${target} PRIVATE
            _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN
        )
        target_link_options(${target} PRIVATE $<$<CONFIG:Release>:/DEBUG>)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endmacro()

# ============================================================
# Target 1: NeoEditor_plugin (DLL)
# 供 PrismaLauncher 运行时加载，跨平台（Windows/Linux/Android）
# 无头模式，不依赖 WinUI3
# ============================================================

set(NEOEDITOR_PLUGIN_NAME "${PROJECT_NAME}_plugin")

add_library(${NEOEDITOR_PLUGIN_NAME} SHARED
    ${NEOEDITOR_PLUGIN_SOURCES}
)

set_target_properties(${NEOEDITOR_PLUGIN_NAME} PROPERTIES
    OUTPUT_NAME "NeoEditor"
    DEBUG_POSTFIX ""
)

setup_target_defaults(${NEOEDITOR_PLUGIN_NAME})
target_compile_definitions(${NEOEDITOR_PLUGIN_NAME} PRIVATE NEOEDITOR_PLUGIN_EXPORTS=1)

# ============================================================
# Target 2: NeoEditor (EXE) — WinUI3 Desktop (Windows-only)
# ============================================================

if(NOT WIN32)
    message(STATUS "NeoEditor EXE: Windows-only (WinUI3), skipping EXE target.")
else()
    add_executable(${PROJECT_NAME}
        # WinUI3 entry point — scaffold only, full implementation in future tasks
    )

    setup_target_defaults(${PROJECT_NAME})

    if(MSVC)
        set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_COMMAND "$<TARGET_FILE:${PROJECT_NAME}>")
    endif()

    # ============================================================
    # 复制 WebUI 资产（Windows EXE）
    # ============================================================

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/webui
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/webui
        COMMENT "Copying WebUI assets"
    )
endif()

# ============================================================
# 复制 WebUI 资产（插件 DLL）— 跨平台
# ============================================================

add_custom_command(TARGET ${NEOEDITOR_PLUGIN_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/webui
        $<TARGET_FILE_DIR:${NEOEDITOR_PLUGIN_NAME}>/assets/webui
    COMMENT "Copying WebUI assets (plugin)"
)
