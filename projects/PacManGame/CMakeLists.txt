cmake_minimum_required(VERSION 3.31)

project(PacManGame
    VERSION 1.0.0
    DESCRIPTION "2D Pac-Man Game built with PrismaEngine"
    LANGUAGES CXX
)

# ========== 编译选项 ==========

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========== 源文件 ==========

set(PACMAN_SOURCES
    src/main.cpp
    src/PacManGame.cpp
    src/game/PacMan.cpp
    src/game/Ghost.cpp
    src/game/GameBoard.cpp
    src/game/GameController.cpp
    src/game/Pellet.cpp
    src/game/PowerPellet.cpp
    src/core/GameResourceManager.cpp
)

set(PACMAN_HEADERS
    src/PacManGame.h
    src/game/PacMan.h
    src/game/Ghost.h
    src/game/GameBoard.h
    src/game/GameController.h
    src/game/Pellet.h
    src/game/PowerPellet.h
    src/core/GameConfig.h
    src/core/GameConstants.h
    src/core/GameResourceManager.h
)

# ========== 可执行文件 ==========

add_executable(${PROJECT_NAME}
    ${PACMAN_SOURCES}
    ${PACMAN_HEADERS}
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
        ${CMAKE_SOURCE_DIR}/../../src/engine
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
            -Werror=return-type
    )
endif()

# ========== 复制资源文件 ==========

# 1. 复制 PacMan 项目自身的资源 (sprites, audios 等)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying PacMan project assets"
)

# 2. 复制引擎必备的着色器资源 (Renderer2D 等)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/shaders
    COMMENT "Copying engine shaders to project output"
)

# ========== 输出信息 ==========

message(STATUS "PacManGame configured")
message(STATUS "  Sources: ${PACMAN_SOURCES}")
