# 代码风格指南

> **状态**: 🔲 规划中
> **版本**: C++20

## 概述

本文档定义 PrismaEngine 项目的 C++ 代码风格规范。

## 命名约定

### 文件命名

| 类型 | 命名规则 | 示例 |
|------|----------|------|
| 头文件 | PascalCase.h | RenderBackend.h |
| 源文件 | PascalCase.cpp | RenderBackend.cpp |
| 模板文件 | PascalCase.inl | VectorUtils.inl |

### 类型命名

| 类型 | 命名规则 | 示例 |
|------|----------|------|
| 类名 | PascalCase | `class ResourceManager` |
| 结构体 | PascalCase | `struct Vector3` |
| 接口 | I + PascalCase | `class IRenderBackend` |
| 枚举 | PascalCase | `enum class RenderAPI` |
| 类型别名 | PascalCase | `using TextureID = uint32_t;` |

### 变量命名

| 类型 | 命名规则 | 示例 |
|------|----------|------|
| 成员变量 | m_camelCase | `m_frameIndex` |
| 局部变量 | camelCase | `vertexCount` |
| 函数参数 | camelCase | `texturePath` |
| 全局变量 | g_camelCase | (避免使用) |
| 常量 | kPascalCase 或 UPPER_SNAKE_CASE | `kMaxFrames` 或 `MAX_FRAMES` |
| constexpr | kPascalCase | `kDefaultWidth` |

### 函数命名

| 类型 | 命名规则 | 示例 |
|------|----------|------|
| 公开方法 | PascalCase | `getResource()` |
| 内部方法 | camelCase 或 _camelCase | `calculateSize()` |
| 访问器 | getPascalCase / setPascalCase | `getWidth() / setWidth()` |
| 布尔返回 | is/has/can/should 前缀 | `isValid()`, `hasTexture()` |

## 代码格式化

### 缩进

- 使用 **4 空格**缩进
- 不使用 Tab

### 大括号

```cpp
// Allman style - 大括号另起一行
class ResourceManager
{
public:
    ResourceManager();
    ~ResourceManager();

    void initialize();
};
```

### 空格

```cpp
// 运算符前后加空格
int result = a + b * c;

// 逗号后加空格
void func(int a, int b, int c);

// 控制流关键字后加空格
if (condition)
{
    doSomething();
}
```

## 注释规范

```cpp
/**
 * @brief 资源管理器
 *
 * 负责加载、缓存和释放引擎资源。
 */
class ResourceManager
{
public:
    /**
     * @brief 加载资源
     * @param path 资源路径
     * @return 资源指针，加载失败返回 nullptr
     */
    std::shared_ptr<Resource> load(const std::string& path);

private:
    // 顶点缓冲区
    ComPtr<ID3D12Resource> m_vertexBuffer;
};
```

## 头文件保护

```cpp
#pragma once

// 或传统方式
#ifndef PRISMA_ENGINE_RENDER_BACKEND_H
#define PRISMA_ENGINE_RENDER_BACKEND_H

// ...

#endif // PRISMA_ENGINE_RENDER_BACKEND_H
```

## 禁止事项

- 禁止使用 `using namespace std;`
- 禁止使用 `malloc/free`，使用 `new/delete` 或智能指针
- 禁止使用 C 风格转换 `(int)ptr`，使用 `static_cast`
- 禁止在头文件中使用 `using` 声明

## 代码组织

```cpp
// 文件结构
#pragma once

// Includes
#include <memory>
#include <vector>

// Forward declarations
class Resource;

namespace Engine
{

// 类声明
class ResourceManager
{
public:
    // 构造/析构
    ResourceManager();
    ~ResourceManager();

    // 公开接口
    // ...

protected:
    // 保护接口
    // ...

private:
    // 私有成员
    // ...

    // 禁止拷贝
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
};

} // namespace Engine
```

## 参考资料

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)

---

*文档创建时间: 2025-12-25*
