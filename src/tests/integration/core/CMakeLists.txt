# Core 系统集成测试
add_prisma_integration_test(test_core_systems
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/EntityManagerTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/NodeTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AssetManagerTest.cpp
)
