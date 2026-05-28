add_prisma_test(test_math
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/MathTypesTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/MurmurHash3Test.cpp
    LINK_LIBS
        glm::glm-header-only
)
target_sources(test_math PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/math/MurmurHash3.cpp
)
