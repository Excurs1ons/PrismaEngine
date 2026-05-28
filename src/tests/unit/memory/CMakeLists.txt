add_prisma_test(test_memory
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/AllocatorTest.cpp
)
target_sources(test_memory PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/memory/MemorySystem.cpp
)
