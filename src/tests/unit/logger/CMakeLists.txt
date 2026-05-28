add_prisma_test(test_logger
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/LoggerTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/LogScopeTest.cpp
)
target_sources(test_logger PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/logger/Logger.cpp
    ${CMAKE_SOURCE_DIR}/src/engine/logger/LogScope.cpp
)
target_include_directories(test_logger PRIVATE
    ${CMAKE_SOURCE_DIR}/.dependencies/SDL3-src/include
)
