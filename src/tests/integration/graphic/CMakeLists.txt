# Graphic 系统集成测试
add_prisma_integration_test(test_graphic
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/RenderTypesTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/MeshTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/MaterialTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/RenderResourceManagerTest.cpp
)
