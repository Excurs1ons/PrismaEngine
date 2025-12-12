#include "TestFramework.h"

// 测试文件会通过REGISTER_TEST_SUITE宏自动注册
// 不需要手动包含

int main() {
    return TestFramework::TestRunner::RunAllSuites();
}