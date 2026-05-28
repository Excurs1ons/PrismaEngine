# PrismaEngine 测试指南

## 1. 启用和构建测试

测试模块默认关闭，通过 CMake 选项 `PRISMA_BUILD_TESTING=ON` 开启。

```bash
# 配置阶段：启用测试
cmake --preset linux-x64-debug -DPRISMA_BUILD_TESTING=ON

# 构建所有测试
cmake --build build/linux-x64-debug --parallel -j2

# 构建单个测试目标（更快）
cmake --build build/linux-x64-debug --target test_sanity -j2
cmake --build build/linux-x64-debug --target test_core_types -j2
cmake --build build/linux-x64-debug --target test_math -j2
```

配置时 `PRISMA_BUILD_TESTING=ON` 会自动触发以下操作：

- 调用 `enable_testing()` 启用 CTest
- `include(cmake/TestingDeps.cmake)` 拉取 Google Test v1.15.2（离线优先，支持 GTest + GMock）
- `add_subdirectory(src/tests)` 加入所有测试目标

---

## 2. 测试组织结构

```
src/tests/
├── CMakeLists.txt              # 顶层测试 CMake：定义 add_prisma_test / add_prisma_integration_test 宏
├── SanityTest.cpp              # 基础验证测试（验证 GTest 可用）
├── AudioTest/                  # 音频集成测试（链接 Engine，需 Vulkan SDK）
│   ├── CMakeLists.txt
│   ├── AudioDSPTest.cpp
│   └── StressTest.cpp
└── unit/                       # 纯逻辑测试（不链接 Engine，无 GPU 依赖）
    ├── CMakeLists.txt           # 分发到子模块
    ├── math/                    # 数学库测试
    ├── core/                    # 核心类型测试（Handle, UUID, Event, Layer 等）
    ├── memory/                  # 内存分配器测试
    ├── logger/                  # 日志系统测试
    ├── serialization/           # 序列化测试（JSON/Binary Archive）
    ├── audio/                   # 音频 DSP 纯逻辑测试（数学计算，节点图等）
    ├── animation/               # 动画系统测试
    ├── ai/                      # AI 系统测试（行为树、状态机、黑板）
    └── navigation/              # 导航系统测试（NavMesh, PathFinder）
```

### 分层含义

| 层级 | 目录 | 说明 | 依赖 | 编译要求 |
|------|------|------|------|----------|
| 纯逻辑测试 | `unit/` | 纯 CPU 逻辑，不涉及 GPU 或窗口 | 仅 GTest + 引擎头文件 | 无 Vulkan SDK 即可编译 |
| 集成测试 | `AudioTest/` | 链接 Engine 共享库，涉及实际子系统 | 链接 Engine + GTest | 需要 Vulkan SDK |
| 基础验证 | `SanityTest.cpp` | 验证 GTest 框架本身可用 | 仅 GTest | 无特殊要求 |

---

## 3. 编写新测试

### 3.1 GTest 基本用法

```cpp
#include <gtest/gtest.h>

// 简单测试
TEST(MathTest, Addition) {
    EXPECT_EQ(1 + 1, 2);
    EXPECT_NE(1 + 1, 3);
    EXPECT_TRUE(1 + 1 == 2);
    EXPECT_FALSE(1 + 1 == 3);
}

// 使用测试夹具（共享初始化/清理）
class MyFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个 TEST_F 执行前调用
    }
    void TearDown() override {
        // 每个 TEST_F 执行后调用
    }
};

TEST_F(MyFixture, SomeTest) {
    EXPECT_GT(42, 0);
}
```

### 3.2 常用断言

| 断言 | 通过条件 |
|------|----------|
| `EXPECT_TRUE(expr)` | `expr` 为 true |
| `EXPECT_FALSE(expr)` | `expr` 为 false |
| `EXPECT_EQ(a, b)` | `a == b` |
| `EXPECT_NE(a, b)` | `a != b` |
| `EXPECT_LT(a, b)` | `a < b` |
| `EXPECT_LE(a, b)` | `a <= b` |
| `EXPECT_GT(a, b)` | `a > b` |
| `EXPECT_GE(a, b)` | `a >= b` |
| `EXPECT_FLOAT_EQ(a, b)` | float 近似相等 |
| `EXPERT_NEAR(a, b, eps)` | `|a - b| <= eps` |
| `EXPECT_STREQ(a, b)` | C 字符串相等 |
| `ASSERT_*` 系列 | 与 `EXPECT_*` 相同，但失败即终止 |

使用 `ASSERT_*` 级联失败时终止当前测试，`EXPECT_*` 则继续执行。

### 3.3 纯逻辑测试模式

纯逻辑测试通过 `add_prisma_test` 宏定义。宏签名：

```cmake
add_prisma_test(<target_name>
    SOURCES
        file1.cpp
        file2.cpp
    LINK_LIBS          # 可选：额外的链接库
        glm::glm-header-only
)
```

宏自动处理：

- 添加 GTest 链接
- 添加引擎头文件搜索路径
- 通过 `add_test()` 注册到 CTest

如果需要编译引擎内部源文件作为测试的一部分，使用 `target_sources()` 追加：

```cmake
add_prisma_test(test_core_types
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/UUIDTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/HandleTest.cpp
)
# 需要额外编译的引擎源文件
target_sources(test_core_types PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/core/UUID.cpp
)
```

如果需要额外的头文件搜索路径或链接库：

```cmake
target_include_directories(test_logger PRIVATE
    ${CMAKE_SOURCE_DIR}/.dependencies/SDL3-src/include
)

target_sources(test_serialization PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/serialization/Serializable.cpp
)
```

> **规则**：纯逻辑测试不能链接 Engine 共享库，不能依赖 Vulkan/GPU。所有被测代码必须通过 `target_sources()` 直接编译，或者只测试头文件中的逻辑。

### 3.4 集成测试模式

集成测试通过 `add_prisma_integration_test` 宏定义。宏签名：

```cmake
add_prisma_integration_test(<target_name>
    SOURCES
        file1.cpp
    LINK_LIBS          # 可选
)
```

集成测试自动链接 Engine 共享库，因此可以调用引擎的全部功能（包括 Vulkan 初始化等），但也因此：

- 需要 Vulkan SDK 可用
- 需要先构建好 Engine 库
- 运行时间较长

### 3.5 Mock 模式

`cmake/TestingDeps.cmake` 默认启用 GMock（`BUILD_GMOCK=ON`）。在需要模拟依赖时：

```cpp
#include <gmock/gmock.h>

class MockRenderer : public IRenderer {
public:
    MOCK_METHOD(void, Draw, (int), (override));
    MOCK_METHOD(int, GetFPS, (), (const, override));
};

TEST(RendererTest, MockExample) {
    MockRenderer mock;
    EXPECT_CALL(mock, Draw(42)).Times(1);
    EXPECT_CALL(mock, GetFPS()).WillOnce(::testing::Return(60));

    mock.Draw(42);
    int fps = mock.GetFPS();
    EXPECT_EQ(fps, 60);
}
```

GMock 是 GTest 的一部分，无需额外依赖。

---

## 4. 运行测试

### 4.1 使用 CTest（推荐）

```bash
# 运行所有测试
ctest --output-on-failure

# 按目标名过滤（支持正则）
ctest --output-on-failure -R test_sanity
ctest --output-on-failure -R test_core
ctest --output-on-failure -R test_math
```

### 4.2 直接运行测试可执行文件

```bash
# 构建目录中找到测试可执行文件
./build/linux-x64-debug/src/tests/test_sanity
./build/linux-x64-debug/src/tests/unit/test_core_types
./build/linux-x64-debug/src/tests/unit/test_math
```

### 4.3 常用 CTest 参数

| 参数 | 说明 |
|------|------|
| `--output-on-failure` | 输出失败测试的详细信息 |
| `-R <regex>` | 只运行名称匹配正则的目标 |
| `-E <regex>` | 排除名称匹配正则的目标 |
| `-N` | 只列出匹配的测试，不运行 |
| `-j<N>` | 并行运行 N 个测试 |
| `--test-dir <dir>` | 指定构建目录 |
| `-V` / `-VV` | 详细输出 |

---

## 5. 添加新测试目标

### 5.1 在已有子模块中添加测试文件

在 `src/tests/unit/<module>/CMakeLists.txt` 中追加源文件：

```cmake
# src/tests/unit/ai/CMakeLists.txt
add_prisma_test(test_ai
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/BehaviorTreeTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FiniteStateMachineTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/BlackboardTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/NewFeatureTest.cpp   # 新增
)
target_sources(test_ai PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/ai/SomeSource.cpp      # 需要时
)
```

### 5.2 创建全新子模块

1. 创建目录和 CMakeLists.txt：

```cmake
# src/tests/unit/newmodule/CMakeLists.txt
add_prisma_test(test_newmodule
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/NewModuleTest.cpp
)
target_sources(test_newmodule PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/newmodule/Source.cpp
)
```

2. 在父层注册：

```cmake
# src/tests/unit/CMakeLists.txt
add_subdirectory(newmodule)       # 新增
```

3. 编写测试源文件：

```cpp
// src/tests/unit/newmodule/NewModuleTest.cpp
#include <gtest/gtest.h>
#include "newmodule/NewModule.h"

namespace Prisma {
namespace {

TEST(NewModuleTest, BasicFunctionality) {
    // 你的测试代码
    EXPECT_EQ(1, 1);
}

} // namespace
} // namespace Prisma
```

### 5.3 添加集成测试

```cmake
# src/tests/CMakeLists.txt 或子目录
add_prisma_integration_test(test_my_integration
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/MyIntegrationTest.cpp
)
```

---

## 6. 规则

1. **不要修改引擎源代码。** 测试代码和被测试代码严格分离。测试文件仅在 `src/tests/` 下。

2. **不要提交 GPU 依赖的测试到 CI。** 纯逻辑测试只能测试 CPU 上的算法和数据结构。所有 CI 构建均使用 `-DPRISMA_BUILD_PROJECT_*_MANAGED=OFF` 跳过示例项目，但不运行 GPU 测试。

3. **纯逻辑测试必须能在没有 Vulkan SDK 的环境下编译。** 不能包含 Vulkan 头文件，不能调用 Vulkan API。这是 Windows CI 和 Android CI 的必要条件。

4. **使用 `namespace Prisma { namespace { ... } }` 包裹测试代码。** 匿名命名空间避免链接冲突。

5. **为每个子系统创建独立的 CMake 目标。** 不要把所有测试塞进一个可执行文件，保持构建粒度细。

6. **测试代码自己负责编译所需的引擎源文件。** 通过 `target_sources()` 引入，不依赖 Engine 共享库。

7. **PRISMA_BUILD_TESTING 默认 OFF。** 这是为了减少大多数开发者的构建时间。开启时需显式传入。

---

## 7. 已知限制

- **集成测试需要先构建 Engine 共享库。** 在 `add_prisma_integration_test` 中链接 Engine 目标自动处理依赖顺序，但 Engine 的构建时间较长。

- **j2 限制下的 Engine 构建时间约 30 分钟以上。** 在资源受限的环境（如 Termux 容器、CI 低配机器）中，使用 `-j2` 构建包含 Engine 的完整测试环境需要约 30 分钟。纯逻辑测试不受此影响——它们仅编译少量源文件，几秒内完成。

- **AudioTest 目前使用集成测试模式**（链接 Engine），后续计划迁移为纯逻辑测试。

- **GTest 通过 FetchContent 拉取。** 首次构建会从 GitHub 下载 googletest v1.15.2（约 5MB）。如果网络受限，可预先将源码放入 `.dependencies/googletest-src/` 目录，CMake 会自动使用本地源码（离线优先策略）。

- **测试模块只在主机平台构建。** Android NDK 交叉编译环境下 `PRISMA_BUILD_TESTING` 被强制设为 `OFF`（通过 CMake 逻辑判断 `ANDROID` 定义）。
