cmake_minimum_required(VERSION 3.20)
project(BlockGame VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find PrismaEngine
find_package(PrismaEngine REQUIRED)

# Create executable
add_executable(BlockGame
    src/main.cpp
    src/Game.cpp
    src/Player.cpp
    src/World.cpp
)

# Link PrismaEngine
target_link_libraries(BlockGame PRIVATE PrismaEngine::Engine)

# Include directories
target_include_directories(BlockGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Copy resources
add_custom_command(TARGET BlockGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:BlockGame>/assets
    COMMENT "Copying assets to output directory"
)
