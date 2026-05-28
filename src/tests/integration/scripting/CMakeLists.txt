# PrismaEngine Scripting 集成测试
# Only build when CoreCLR scripting is enabled
if(PRISMA_ENABLE_SCRIPTING STREQUAL "CORECLR")
    add_prisma_integration_test(test_scripting
        SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/CoreCLRHostTest.cpp
    )
endif()
