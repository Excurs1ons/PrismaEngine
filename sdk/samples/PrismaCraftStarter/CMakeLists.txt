cmake_minimum_required(VERSION 3.20)
project(PrismaCraftStarter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Find PrismaEngine SDK ──
#   cmake -B build -DPrismaEngine_DIR=/path/to/PrismaEngine-SDK-<version>-<platform>
# ──────────────────────────
find_package(PrismaEngine REQUIRED)

# Find PrismaCraft (if installed separately)
# find_package(PrismaCraft REQUIRED)

add_executable(PrismaCraftStarter
    src/main.cpp
    src/MinecraftGame.cpp
)

target_link_libraries(PrismaCraftStarter PRIVATE
    PrismaEngine::Engine
    # PrismaCraft::PrismaCraft
)

target_include_directories(PrismaCraftStarter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PRISMAENGINE_INCLUDE_DIR}
    # /path/to/PrismaCraft/include
)

# Copy resources
add_custom_command(TARGET PrismaCraftStarter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:PrismaCraftStarter>/assets
    COMMENT "Copying assets to output directory"
)
