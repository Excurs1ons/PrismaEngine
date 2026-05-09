cmake_minimum_required(VERSION 3.20)

project(Scene2DTest
    VERSION 1.0.0
    DESCRIPTION "2D Scene Test Template for PrismaEngine"
    LANGUAGES CXX
)

# ========== 编译选项 ==========

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========== 源文件 ==========

set(TEST_SOURCES
    src/main.cpp
    src/Scene2DTestApp.cpp
)

set(TEST_HEADERS
    src/Scene2DTestApp.h
)

# ========== 可执行文件 ==========

add_executable(${PROJECT_NAME}
    ${TEST_SOURCES}
    ${TEST_HEADERS}
)

# ========== 链接引擎 ==========

target_link_libraries(${PROJECT_NAME} PRIVATE
    Engine
    SDL3::SDL3
)

# ========== 包含目录 ==========

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# ========== 编译选项 ==========

if(MSVC)
    target_compile_options(${PROJECT_NAME}
        PRIVATE
            /W4
            /WX-
            /utf-8
    )
else()
    target_compile_options(${PROJECT_NAME}
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

# ========== 复制引擎着色器 ==========

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/shaders
    COMMENT "Copying engine shaders to project output"
)

message(STATUS "Scene2DTest configured")
