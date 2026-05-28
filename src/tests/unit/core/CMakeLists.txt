add_prisma_test(test_core_types
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/UUIDTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/HandleTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/TimestepTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/StringHashTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ComponentTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/EventTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/LayerTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/LayerStackTest.cpp
)
target_sources(test_core_types PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/core/UUID.cpp
    ${CMAKE_SOURCE_DIR}/src/engine/core/Layer.cpp
    ${CMAKE_SOURCE_DIR}/src/engine/core/LayerStack.cpp
)
