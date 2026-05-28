# PrismaEngine Terrain 集成测试
add_prisma_integration_test(test_terrain
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/HeightMapTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/TerrainCollisionTest.cpp
)
