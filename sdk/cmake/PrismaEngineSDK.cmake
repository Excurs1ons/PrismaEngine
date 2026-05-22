# PrismaEngineSDK.cmake
# SDK helper module — included by PrismaEngineConfig.cmake
# Provides SDK-specific utility functions for projects using the SDK.

# Add all PrismaEngine module subdirectories to the include path
# This mirrors how the engine's own CMakeLists.txt sets up includes,
# ensuring internal includes like #include "Export.h" or
# #include "ISubSystem.h" resolve correctly.
macro(prisma_setup_sdk_includes TARGET)
    if(PRISMAENGINE_INCLUDE_DIR)
        target_include_directories(${TARGET} PRIVATE
            "${PRISMAENGINE_INCLUDE_DIR}"
            "${PRISMAENGINE_INCLUDE_DIR}/PrismaEngine"
        )

        # Add all subdirectories for flat include resolution
        file(GLOB _PRISMA_SDK_SUBDIRS
            "${PRISMAENGINE_INCLUDE_DIR}/PrismaEngine/*/"
        )
        foreach(_DIR ${_PRISMA_SDK_SUBDIRS})
            target_include_directories(${TARGET} PRIVATE "${_DIR}")
        endforeach()
    endif()
endmacro()

# Copy engine runtime assets (shaders, textures, configs) to output
macro(prisma_copy_assets TARGET)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PRISMAENGINE_SDK_DIR}/shaders"
            "$<TARGET_FILE_DIR:${TARGET}>/shaders"
        COMMENT "Copying PrismaEngine shaders to output"
    )
endmacro()
