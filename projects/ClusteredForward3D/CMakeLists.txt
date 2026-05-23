cmake_minimum_required(VERSION 3.20)

project(ClusteredForward3D
    VERSION 1.0.0
    DESCRIPTION "Clustered Forward Rendering implementation project for PrismaEngine"
    LANGUAGES CXX
)

# ========== 编译选项 ==========

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========== 源码 ==========

set(PROJECT_SOURCES
    src/main.cpp
    src/ClusteredApp.cpp
    src/StatsOverlay.cpp
)

set(PROJECT_HEADERS
    src/ClusteredApp.h
    src/StatsOverlay.h
)

# ========== 包含目录 ==========

set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ========== 默认设置宏 ==========

macro(setup_target_defaults target)
    target_include_directories(${target} PRIVATE ${PROJECT_INCLUDE_DIRS})
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
# Target: ClusteredForward3D (EXE)
# ============================================================

add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

setup_target_defaults(${PROJECT_NAME})

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_COMMAND "$<TARGET_FILE:${PROJECT_NAME}>")
endif()

# ============================================================
# 复制项目资源
# ============================================================

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/ClusteredForward3D.jsonc
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/ClusteredForward3D.jsonc
    COMMENT "Copying shaders and project assets"
)
