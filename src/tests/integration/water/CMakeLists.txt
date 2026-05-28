# PrismaEngine Water 集成测试
add_prisma_integration_test(test_water
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/WaveSimulationTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/WaterInteractionTest.cpp
)
