# Audio 系统集成测试
add_prisma_integration_test(test_audio
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/AudioAPITest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/SoundSynthesizerTest.cpp
    LINK_LIBS
        GMock::GMock
)
