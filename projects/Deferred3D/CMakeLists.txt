cmake_minimum_required(VERSION 3.20)

project(Deferred3D
    VERSION 1.0.0
    DESCRIPTION "Deferred 3D Rendering Debug Project for PrismaEngine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DEFERRED3D_SOURCES
    src/Deferred3DApp.cpp
    src/DeferredPipelineAdapter.cpp
    src/StatsOverlay.cpp
)

set(DEFERRED3D_HEADERS
    src/Deferred3DApp.h
    src/DeferredPipelineAdapter.h
    src/StatsOverlay.h
)

set(DEFERRED3D_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

macro(setup_target_defaults target)
    target_include_directories(${target} PRIVATE ${DEFERRED3D_INCLUDE_DIRS})
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

add_executable(${PROJECT_NAME}
    src/main.cpp
    ${DEFERRED3D_SOURCES}
    ${DEFERRED3D_HEADERS}
)

setup_target_defaults(${PROJECT_NAME})

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_COMMAND "$<TARGET_FILE:${PROJECT_NAME}>")
endif()

set(DEFERRED3D_PLUGIN_NAME "${PROJECT_NAME}_plugin")

add_library(${DEFERRED3D_PLUGIN_NAME} SHARED
    src/CreateApplication.cpp
    ${DEFERRED3D_SOURCES}
    ${DEFERRED3D_HEADERS}
)

set_target_properties(${DEFERRED3D_PLUGIN_NAME} PROPERTIES
    OUTPUT_NAME "Deferred3D"
    DEBUG_POSTFIX ""
)

setup_target_defaults(${DEFERRED3D_PLUGIN_NAME})
target_compile_definitions(${DEFERRED3D_PLUGIN_NAME} PRIVATE DEFERRED3D_PLUGIN_EXPORTS=1)

# GLSL → SPIR-V 自动编译
find_program(GLSLANG_VALIDATOR glslangValidator)
if(GLSLANG_VALIDATOR)
    # 项目自身的着色器
    file(GLOB SHADER_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/*.frag"
    )
    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SPV_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/${SHADER_NAME}.spv")
        add_custom_command(
            OUTPUT ${SPV_OUTPUT}
            COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER} -o ${SPV_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling ${SHADER_NAME} → SPIR-V"
        )
        list(APPEND GENERATED_SPVS ${SPV_OUTPUT})
    endforeach()

    add_custom_target(CompileDeferredShaders DEPENDS ${GENERATED_SPVS})
    add_dependencies(${PROJECT_NAME} CompileDeferredShaders AssetShaders)
    add_dependencies(${DEFERRED3D_PLUGIN_NAME} CompileDeferredShaders AssetShaders)
    message(STATUS "Deferred3D: GLSL→SPIR-V 编译已启用")
else()
    message(WARNING "glslangValidator 未找到 - PBR 着色器需手动编译为 SPIR-V")
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/Deferred3D.jsonc
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/Deferred3D.jsonc
    COMMENT "Copying shaders and project assets"
)

add_custom_command(TARGET ${DEFERRED3D_PLUGIN_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/shaders
        $<TARGET_FILE_DIR:${DEFERRED3D_PLUGIN_NAME}>/assets/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${DEFERRED3D_PLUGIN_NAME}>/assets
    COMMENT "Copying shaders and project assets (plugin)"
)
