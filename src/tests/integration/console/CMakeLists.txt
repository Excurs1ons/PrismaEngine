# Console 系统集成测试
add_prisma_integration_test(test_console
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/CVarTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ConsoleCommandTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ConsoleSystemTest.cpp
)
