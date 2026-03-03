# PrismaEngine & PrismaCraft 架构优化指南

## 概述

本文档描述了 PrismaEngine 和 PrismaCraft 的架构优化计划，包括 API 统一、性能优化和代码组织改进。

## API 设计原则

### 1. 命名约定

#### 方法命名
- **使用 camelCase**: `getName()`, `setPosition()`, `updateWorld()`
- **避免 C 风格**: 不要使用 `get_name()`, `set_position()`

```cpp
// ✅ 好的做法
class GameObject {
    std::string getName() const;
    void setPosition(const glm::vec3& pos);
    void updateWorld(float deltaTime);
};

// ❌ 避免
class GameObject {
    std::string get_name() const;  // C 风格
    void SetPosition(const glm::vec3& pos);  // PascalCase
};
```

#### 类命名
- **使用 PascalCase**: `GameObject`, `RenderDevice`, `WorldGenerator`
- **接口前缀**: 可选使用 `I` 前缀，如 `IRenderDevice`

```cpp
// ✅ 好的做法
class RenderDevice { /* ... */ };
class IRenderDevice { /* ... */ };  // 接口

// ❌ 避免
class render_device { /* ... */ };  // snake_case
class renderDevice { /* ... */ };   // 混合
```

### 2. const 正确性

#### Getter 方法
所有 getter 方法必须是 const：

```cpp
// ✅ 好的做法
class Block {
    const std::string& getName() const;
    int getId() const;
    bool isSolid() const;
};

// ❌ 避免
class Block {
    const std::string& getName();  // 非 const
};
```

#### 参数传递
- 复杂类型使用 const 引用
- 简单类型使用值传递

```cpp
// ✅ 好的做法
void processBlock(const BlockState& state);  // const 引用
void setAge(int age);  // 值传递

// ❌ 避免
void processBlock(BlockState state);  // 不必要的拷贝
void setAge(const int& age);  // 简单类型不需要引用
```

### 3. 智能指针使用

#### 返回值
- 返回共享所有权使用 `std::shared_ptr`
- 返回独占所有权使用 `std::unique_ptr`
- 返回引用或观察者使用原始指针

```cpp
// ✅ 好的做法
class AssetManager {
    // 共享所有权
    std::shared_ptr<Texture> loadTexture(const std::string& path);

    // 观察者（不拥有）
    Texture* getTexture(const std::string& id) const;
};

// ❌ 避免
class AssetManager {
    Texture* loadTexture(const std::string& path);  // 所有权不明确
};
```

#### 成员变量
- 独占所有权使用 `std::unique_ptr`
- 共享所有权使用 `std::shared_ptr`
- 观察者使用原始指针

```cpp
// ✅ 好的做法
class GameObject {
    std::unique_ptr<Transform> transform_;  // 独占
    std::vector<std::shared_ptr<Component>> components_;  // 共享
    World* world_;  // 观察者（不拥有）
};
```

### 4. 异常安全

#### RAII 原则
使用 RAII 管理资源：

```cpp
// ✅ 好的做法
class FileManager {
    std::fstream file_;
public:
    FileManager(const std::string& path) : file_(path) {}
    ~FileManager() { /* 自动关闭 */ }
};

// ❌ 避免
class FileManager {
    FILE* file_;
public:
    FileManager(const std::string& path) {
        file_ = fopen(path.c_str(), "r");
    }
    ~FileManager() {
        if (file_) fclose(file_);  // 手动管理
    }
};
```

#### 异常保证
提供强异常保证：

```cpp
// ✅ 好的做法
class Level {
    void setBlock(const BlockPos& pos, const BlockState& state) {
        // 先在局部修改，成功后再提交
        BlockState oldState = getBlockState(pos);
        blocks_[pos] = state;
        // 如果后续操作失败，可以回滚
    }
};
```

## 性能优化

### 1. 内存管理

#### 对象池
为频繁创建/销毁的对象使用对象池：

```cpp
// Entity 对象池
template<typename T>
class ObjectPool {
public:
    T* allocate() {
        if (!freeList_.empty()) {
            auto* obj = freeList_.back();
            freeList_.pop_back();
            return obj;
        }
        return &pool_.emplace_back();
    }

    void deallocate(T* obj) {
        obj->~T();
        freeList_.push_back(obj);
    }

private:
    std::vector<T> pool_;
    std::vector<T*> freeList_;
};

// 使用
ObjectPool<Entity> entityPool;
auto* entity = entityPool.allocate();
// ... 使用 ...
entityPool.deallocate(entity);
```

#### 内存分配策略
- 预分配容器
- 使用 reserve()
- 避免频繁分配

```cpp
// ✅ 好的做法
std::vector<Block*> blocks;
blocks.reserve(1024);  // 预分配

// ❌ 避免
std::vector<Block*> blocks;
// 在循环中反复 push_back 导致多次重分配
```

### 2. 多线程优化

#### 任务并行
使用线程池并行处理任务：

```cpp
class ChunkGenerationSystem {
    ThreadPool threadPool_;

public:
    void generateChunk(LevelChunk* chunk) {
        threadPool_.enqueue([this, chunk]() {
            // 异步生成区块
            generateTerrain(chunk);
            generateCaves(chunk);
        });
    }
};
```

#### 数据局部性
优化数据布局以提高缓存命中率：

```cpp
// ✅ 好的做法 - SoA (Structure of Arrays)
struct ChunkData {
    std::array<uint16_t, 4096> blockIds;
    std::array<uint8_t, 4096> blockData;
};

// ❌ 避免 - AoS (Array of Structures) - 缓存不友好
struct BlockData {
    uint16_t id;
    uint8_t data;
    uint16_t padding;  // 填充
};
std::array<BlockData, 4096> blocks;
```

### 3. 渲染优化

#### 批量渲染
合并绘制调用：

```cpp
class BlockRenderer {
    // 收集可见方块
    std::vector<BlockRenderData> visibleBlocks_;

    // 批量渲染
    void renderBatch() {
        if (visibleBlocks_.empty()) return;

        // 一次性上传所有顶点数据
        uploadVertexData(visibleBlocks_);

        // 单次绘制调用
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
    }
};
```

#### 视锥剔除
只渲染可见对象：

```cpp
void renderScene(const Frustum& frustum) {
    for (auto& chunk : chunks_) {
        if (!frustum.isVisible(chunk->getBounds())) {
            continue;  // 跳过不可见区块
        }
        renderChunk(chunk);
    }
}
```

## 代码组织

### 推荐目录结构

```
PrismaEngine/src/engine/
├── core/              # 核心系统
│   ├── ECS.h
│   ├── AssetManager.h
│   └── AsyncLoader.h
├── graphic/           # 渲染系统
│   ├── interfaces/    # 渲染接口
│   ├── adapters/      # 平台适配器
│   └── pipelines/     # 渲染管线
├── physics/           # 物理系统
│   ├── CollisionSystem.h
│   └── RigidBody.h
├── audio/             # 音频系统
│   ├── AudioManager.h
│   └── AudioDevice.h
├── input/             # 输入系统
│   ├── InputManager.h
│   └── InputDevice.h
└── platform/          # 平台特定代码
    ├── PlatformWindows.cpp
    └── PlatformLinux.cpp
```

### 模块化设计

#### 依赖注入
使用依赖注入减少耦合：

```cpp
// ✅ 好的做法
class Game {
    Level* level_;  // 注入依赖
    InputManager* input_;
    RenderDevice* render_;

public:
    Game(Level* level, InputManager* input, RenderDevice* render)
        : level_(level), input_(input), render_(render) {}
};

// ❌ 避免 - 紧耦合
class Game {
    std::unique_ptr<Level> level_;  // 创建并拥有
    std::unique_ptr<InputManager> input_;
    std::unique_ptr<RenderDevice> render_;
};
```

#### 接口隔离
定义清晰的接口：

```cpp
// 渲染接口
class IRenderable {
public:
    virtual void render(RenderDevice* device) = 0;
};

// 更新接口
class IUpdatable {
public:
    virtual void update(float deltaTime) = 0;
};

// 多继承
class GameObject : public IRenderable, public IUpdatable {
    // ...
};
```

## 迁移检查清单

- [ ] 所有 getter 方法添加 const
- [ ] 方法名统一为 camelCase
- [ ] 类名统一为 PascalCase
- [ ] 参数使用 const 引用（复杂类型）
- [ ] 返回值使用智能指针（所有权转移）
- [ ] 实现对象池（频繁创建的对象）
- [ ] 使用线程池（耗时任务）
- [ ] 实现视锥剔除（渲染优化）
- [ ] 优化数据布局（缓存友好）
- [ ] 添加异常处理
- [ ] 统一错误处理机制

## 参考

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Effective C++](https://www.aristeia.com/books.html)
- [Game Engine Architecture](https://www.gameenginebook.com/)
