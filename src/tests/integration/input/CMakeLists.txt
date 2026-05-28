# Input 系统集成测试
add_prisma_integration_test(test_input
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/InputManagerTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/InputDeviceTest.cpp
    LINK_LIBS
        GMock::GMock
)
