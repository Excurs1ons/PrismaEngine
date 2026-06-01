# MCP JSON-RPC 格式纯逻辑测试
# 需要 glaze 头文件路径
add_prisma_test(test_mcp_tools
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/MCPToolTest.cpp
)
target_include_directories(test_mcp_tools PRIVATE
    ${CMAKE_SOURCE_DIR}/.dependencies/glaze-src/include
)
