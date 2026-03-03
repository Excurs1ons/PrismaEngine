cmake_minimum_required(VERSION 3.20)
project(PrismaCraftStarter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find PrismaEngine
find_package(PrismaEngine REQUIRED)

# Find PrismaCraft (if installed separately)
# find_package(PrismaCraft REQUIRED)

# Create executable
add_executable(PrismaCraftStarter
    src/main.cpp
    src/MinecraftGame.cpp
)

# Link libraries
target_link_libraries(PrismaCraftStarter PRIVATE
    PrismaEngine::Engine
    # PrismaCraft::PrismaCraft  # 如果使用独立的 PrismaCraft
)

# Include directories
target_include_directories(PrismaCraftStarter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # /path/to/PrismaCraft/include  # 如果使用独立的 PrismaCraft
)

# Copy resources
add_custom_command(TARGET PrismaCraftStarter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:PrismaCraftStarter>/assets
    COMMENT "Copying assets to output directory"
)
