# PrismaEngine Network 集成测试
add_prisma_integration_test(test_network
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PacketTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/SessionTest.cpp
    LINK_LIBS
        GMock::GMock
)
