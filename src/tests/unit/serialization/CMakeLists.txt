add_prisma_test(test_serialization
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/ArchiveJsonTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ArchiveBinaryTest.cpp
    LINK_LIBS
        glaze::glaze
        glm::glm-header-only
)
target_include_directories(test_serialization PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/serialization
    ${CMAKE_SOURCE_DIR}/src/engine/resource
)
target_sources(test_serialization PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/serialization/Serializable.cpp
    ${CMAKE_SOURCE_DIR}/src/engine/resource/ArchiveJson.cpp
)
