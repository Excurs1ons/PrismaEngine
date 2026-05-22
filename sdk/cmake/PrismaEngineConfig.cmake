# PrismaEngine SDK CMake Configuration
#
# This config file supports TWO modes:
#
# Mode 1 — Local Repo Development:
#   cmake -B build -DPrismaEngine_DIR=/path/to/PrismaEngine/sdk
#   - Uses headers from sdk/include/
#   - Builds engine from source (requires full repo)
#   - Use when you have the full PrismaEngine repository
#
# Mode 2 — Prebuilt SDK (GitHub Release):
#   cmake -B build -DPrismaEngine_DIR=/path/to/downloaded-sdk
#   - Uses headers from downloaded SDK
#   - Links against prebuilt libraries in lib/<platform>/
#   - Use when you downloaded a release from GitHub
#
# Usage in your CMakeLists.txt:
#   find_package(PrismaEngine REQUIRED)
#   target_link_libraries(my_game PRIVATE PrismaEngine::Engine)
#   target_include_directories(my_game PRIVATE ${PRISMAENGINE_INCLUDE_DIR})

# ── Detect Mode ──
# If we find prebuilt libraries, we're in Release mode.
# Otherwise, we're in development mode.

# Set up paths
get_filename_component(PRISMAENGINE_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(PRISMAENGINE_INCLUDE_DIR "${PRISMAENGINE_SDK_DIR}/include")

# Check for prebuilt libraries
if(WIN32)
    set(_PRISMA_LIB_SEARCH "${PRISMAENGINE_SDK_DIR}/lib/windows")
elseif(ANDROID)
    set(_PRISMA_LIB_SEARCH "${PRISMAENGINE_SDK_DIR}/lib/android")
else()
    set(_PRISMA_LIB_SEARCH "${PRISMAENGINE_SDK_DIR}/lib/linux")
endif()

# Look for the engine library
find_library(PRISMAENGINE_LIBRARY
    NAMES Engine PrismaEngine prisma_engine
    PATHS "${_PRISMA_LIB_SEARCH}"
    NO_DEFAULT_PATH
)

if(PRISMAENGINE_LIBRARY)
    # ── Mode 2: Prebuilt SDK ──
    message(STATUS "PrismaEngine SDK: Using prebuilt libraries from ${_PRISMA_LIB_SEARCH}")

    add_library(PrismaEngine::Engine SHARED IMPORTED)
    set_target_properties(PrismaEngine::Engine PROPERTIES
        IMPORTED_LOCATION "${PRISMAENGINE_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PRISMAENGINE_INCLUDE_DIR}"
    )

    if(WIN32 AND PRISMAENGINE_LIBRARY MATCHES "\\.lib$")
        # On Windows, find the corresponding DLL
        string(REGEX REPLACE "\\.lib$" ".dll" _PRISMA_DLL_PATH "${PRISMAENGINE_LIBRARY}")
        if(EXISTS "${_PRISMA_DLL_PATH}")
            set_target_properties(PrismaEngine::Engine PROPERTIES
                IMPORTED_IMPLIB "${PRISMAENGINE_LIBRARY}"
            )
        endif()
    endif()

else()
    # ── Mode 1: Local Repo Development ──
    # Check if we're inside the PrismaEngine repo
    if(EXISTS "${PRISMAENGINE_SDK_DIR}/../CMakeLists.txt" AND
       EXISTS "${PRISMAENGINE_SDK_DIR}/../src/engine/CMakeLists.txt")

        message(STATUS "PrismaEngine SDK: Building from source (development mode)")
        message(STATUS "PrismaEngine SDK: Run 'cmake --preset <platform>' in the repo root first")

        # Forward to the main build
        add_subdirectory("${PRISMAENGINE_SDK_DIR}/../src/engine" "${CMAKE_BINARY_DIR}/prisma-engine-build")
        add_library(PrismaEngine::Engine ALIAS Engine)

    else()
        # Not in repo and no prebuilt libs — error with instructions
        message(FATAL_ERROR "
╔══════════════════════════════════════════════════════════════╗
║  PrismaEngine SDK: Prebuilt libraries not found!            ║
║                                                              ║
║  To build your project, you need either:                     ║
║                                                              ║
║  Option A — Download Prebuilt SDK (Recommended):             ║
║    https://github.com/Excurs1ons/PrismaEngine/releases       ║
║    Extract and set: -DPrismaEngine_DIR=/path/to/sdk          ║
║                                                              ║
║  Option B — Build from Source:                               ║
║    git clone https://github.com/Excurs1ons/PrismaEngine.git  ║
║    cd PrismaEngine && cmake --preset <platform>              ║
║    cmake --build build/<preset>                              ║
║    Then set: -DPrismaEngine_DIR=/path/to/PrismaEngine/sdk    ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
")
    endif()
endif()

# ── Set public variables ──
set(PRISMAENGINE_FOUND TRUE)
set(PRISMAENGINE_VERSION "0.1.0")
set(PRISMAENGINE_SDK_MODE "release")  # 'development' or 'release'

# ── SDK Helper Functions ──

# prisma_create_app: Create a PrismaEngine application target
function(prisma_create_app APP_NAME)
    cmake_parse_arguments(ARG "" "FOLDER" "SOURCES;LIBRARIES" ${ARGN})

    add_executable(${APP_NAME})

    if(ARG_SOURCES)
        target_sources(${APP_NAME} PRIVATE ${ARG_SOURCES})
    endif()

    set_target_properties(${APP_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_link_libraries(${APP_NAME} PRIVATE PrismaEngine::Engine)
    target_include_directories(${APP_NAME} PRIVATE ${PRISMAENGINE_INCLUDE_DIR})

    if(ARG_LIBRARIES)
        target_link_libraries(${APP_NAME} PRIVATE ${ARG_LIBRARIES})
    endif()

    if(ARG_FOLDER)
        set_target_properties(${APP_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${ARG_FOLDER}
        )
    endif()
endfunction()

# prisma_create_editor_extension: Create a PrismaEngine Editor extension
function(prisma_create_editor_extension EXTENSION_NAME)
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})

    add_library(${EXTENSION_NAME} SHARED ${ARG_SOURCES})

    set_target_properties(${EXTENSION_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_link_libraries(${EXTENSION_NAME} PRIVATE PrismaEngine::Engine)
    target_include_directories(${EXTENSION_NAME} PRIVATE ${PRISMAENGINE_INCLUDE_DIR})
    target_compile_definitions(${EXTENSION_NAME} PRIVATE EDITOR_EXTENSION_EXPORTS=1)
endfunction()
