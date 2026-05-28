# Threading 系统集成测试
add_prisma_integration_test(test_threading
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/JobSystemTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ThreadManagerTest.cpp
)
