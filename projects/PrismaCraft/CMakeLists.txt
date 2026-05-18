cmake_minimum_required(VERSION 3.20)

project(PrismaCraft
    VERSION 1.0.0
    DESCRIPTION "PrismaCraft - C#-based Minecraft game with SRP"
    LANGUAGES CXX
)

# ========== 编译选项 ==========

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========== 共享源码（EXE 与 Plugin 共用） ==========

set(PRISMACRAFT_SOURCES
    src/PrismaCraftApp.cpp
)

set(PRISMACRAFT_HEADERS
    src/PrismaCraftApp.h
)

# ========== 共享包含目录 ==========

set(PRISMACRAFT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/engine/app
    ${CMAKE_SOURCE_DIR}/src/engine/core
    ${CMAKE_SOURCE_DIR}/src/engine/graphic
    ${CMAKE_SOURCE_DIR}/src/engine/logger
    ${CMAKE_SOURCE_DIR}/src/engine/platform
    ${CMAKE_SOURCE_DIR}/src/engine/scripting
    ${CMAKE_SOURCE_DIR}/src/engine/window
    ${CMAKE_SOURCE_DIR}/src/engine/math
    ${CMAKE_SOURCE_DIR}/src/engine/resource
    ${CMAKE_SOURCE_DIR}/src/engine/audio
    ${CMAKE_SOURCE_DIR}/src/engine/ui
    ${CMAKE_SOURCE_DIR}/src/engine/ui/2d
    ${CMAKE_SOURCE_DIR}/src/engine/input
    ${CMAKE_SOURCE_DIR}/src/engine/tilemap
    ${CMAKE_SOURCE_DIR}/src/engine/scene
    ${CMAKE_SOURCE_DIR}/src/engine/object
    ${CMAKE_SOURCE_DIR}/src/engine/transform
    ${CMAKE_SOURCE_DIR}/src/engine/threading
    ${CMAKE_SOURCE_DIR}/src/engine/serialization
    ${CMAKE_SOURCE_DIR}/src/engine/physics
    ${CMAKE_SOURCE_DIR}/src/engine/config
    ${CMAKE_SOURCE_DIR}/src/engine/packing
    ${CMAKE_SOURCE_DIR}/src/engine/utils
)

# ========== 共享编译选项宏 ==========

macro(setup_target_defaults target)
    target_include_directories(${target} PRIVATE ${PRISMACRAFT_INCLUDE_DIRS})
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
# Target 1: PrismaCraft (EXE)
# 独立可执行文件，直接链接 Engine.dll，F5 即可调试
# ============================================================

add_executable(${PROJECT_NAME}
    src/main.cpp
    ${PRISMACRAFT_SOURCES}
    ${PRISMACRAFT_HEADERS}
)

setup_target_defaults(${PROJECT_NAME})

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_COMMAND "$<TARGET_FILE:${PROJECT_NAME}>")
endif()

# ============================================================
# Target 2: PrismaCraft_plugin (DLL)
# 保持与旧 Launcher 加载路径兼容，输出 PrismaCraft.dll
# ============================================================

set(PRISMACRAFT_PLUGIN_NAME "${PROJECT_NAME}_plugin")

add_library(${PRISMACRAFT_PLUGIN_NAME} SHARED
    src/CreateApplication.cpp
    ${PRISMACRAFT_SOURCES}
    ${PRISMACRAFT_HEADERS}
)

set_target_properties(${PRISMACRAFT_PLUGIN_NAME} PROPERTIES
    OUTPUT_NAME "PrismaCraft"              # 生成 PrismaCraft.dll（与旧名一致）
    DEBUG_POSTFIX ""                        # Debug 也不加后缀
)

setup_target_defaults(${PRISMACRAFT_PLUGIN_NAME})
target_compile_definitions(${PRISMACRAFT_PLUGIN_NAME} PRIVATE PRISMACRAFT_PLUGIN_EXPORTS=1)

# ============================================================
# 复制项目资源
# ============================================================

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/shaders
    COMMENT "Copying shaders"
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
        COMMENT "Copying project assets"
    )
endif()

add_custom_command(TARGET ${PRISMACRAFT_PLUGIN_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/shaders
        $<TARGET_FILE_DIR:${PRISMACRAFT_PLUGIN_NAME}>/assets/shaders
    COMMENT "Copying shaders (plugin)"
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(TARGET ${PRISMACRAFT_PLUGIN_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:${PRISMACRAFT_PLUGIN_NAME}>/assets
        COMMENT "Copying project assets (plugin)"
    )
endif()

# ============================================================
# C# 脚本构建与部署
# ============================================================

if(PRISMA_ENABLE_SCRIPTING)
    add_dependencies(${PROJECT_NAME} PrismaCore)
    add_dependencies(${PRISMACRAFT_PLUGIN_NAME} PrismaCore)

    set(CS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts/lib")
    find_program(DOTNET dotnet REQUIRED)

    if(WIN32)
        set(CS_RID "win-x64")
    else()
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
            set(CS_RID "linux-arm64")
        else()
            set(CS_RID "linux-x64")
        endif()
    endif()

    set(SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
    set(SCRIPTS_OUT "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Publishing C# scripts for RID: ${CS_RID}..."
        COMMAND ${DOTNET} publish -c Release -r ${CS_RID} --self-contained true
                -p:DebugType=portable -p:DebugSymbols=true
                -p:PrismaCoreRef="${CS_LIB_DIR}"
                --nologo
                "${SCRIPTS_DIR}/GameScripts/GameScripts.csproj"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${SCRIPTS_DIR}/GameScripts/bin/Release/net10.0/${CS_RID}/publish/"
                "${SCRIPTS_OUT}/"
        COMMENT "Building + publishing C# scripts (RID: ${CS_RID})"
    )

    message(STATUS "PrismaCraft configured for RID: ${CS_RID}")
else()
    message(STATUS "PrismaCraft: Scripting disabled, skipping C# dependencies.")
endif()
