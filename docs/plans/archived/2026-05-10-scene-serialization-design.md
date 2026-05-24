# 场景序列化设计文档

## 目标

让 Component/GameObject/Transform/Scene 完整支持 JSONC 格式的正反序列化，使场景系统可用。

## JSONC 格式

```jsonc
{
  "name": "MyScene",
  "camera": {
    "projection": [0.0, 1920.0, 0.0, 1080.0]
  },
  "gameObjects": [
    {
      "name": "Player",
      "transform": {
        "position": [0.0, 0.0, 0.0],
        "rotation": [0.0, 0.0, 0.0, 1.0],  // 四元数 x,y,z,w
        "scale": [1.0, 1.0, 1.0]
      },
      "components": [
        {
          "type": "SpriteRenderer",
          "data": {
            "color": [1.0, 0.2, 0.2, 1.0],
            "position": [200.0, 200.0],
            "size": [100.0, 100.0],
            "rotation": 45.0
          }
        }
      ]
    }
  ]
}
```

- 使用 `.jsonc` 扩展名，支持注释
- `glz::read_file_jsonc` 读取，`glz::write_file_json` 写入（生成纯 JSON）
- Transform 内联在 GameObject 中，不单独作为 Component

## 架构

### 1. ComponentRegistry（新增）

类型安全的组件工厂注册表，字符串标识符 → 工厂函数。

```cpp
class ComponentRegistry {
    static ComponentRegistry& Get();
    template<typename T> void Register(const char* typeName);
    std::shared_ptr<Component> Create(const std::string& typeName);
    std::string_view GetTypeName(const Component& component) const;
};
```

每个可序列化的 Component 子类在 `.cpp` 中静态注册。

### 2. Component 基类

加一个纯虚方法：

```cpp
virtual const char* GetComponentTypeName() const = 0;
```

### 3. Component Data 模式

每个可序列化的 Component 子类定义嵌套 `Data` 纯数据 struct，通过 `GetData()`/`SetData()` 与组件交换数据。

```cpp
class SpriteRenderer : public Component {
public:
    struct Data {
        Color color = {1,1,1,1};
        Vector2 position = {0,0};
        Vector2 size = {1,1};
        float rotation = 0.0f;
    };
    const char* GetComponentTypeName() const override { return "SpriteRenderer"; }
    Data GetData() const;
    void SetData(const Data& d);
};
```

Data 与 `glz::generic` 的转换通过 JSON 字符串做桥接：
- 序列化: `write_json(data, string)` → `read_json(generic, string)`
- 反序列化: `generic.dump()` → string → `read_json(data, string)`

### 4. GameObject 序列化

```cpp
struct ComponentEntry {
    std::string type;
    glz::generic data;
};
struct GameObjectData {
    std::string name;
    std::array<float, 3> position;
    std::array<float, 4> rotation;
    std::array<float, 3> scale;
    std::vector<ComponentEntry> components;
};
```

GameObject 的 `GetData()` 遍历所有 Component，查注册表获取 type name，调用 `GetData()` 获取 Data struct 后转为 `glz::generic`。

### 5. Scene 序列化

```cpp
struct SceneFileData {
    std::string name;
    struct { std::array<float, 4> projection; } camera;
    std::vector<GameObjectData> gameObjects;
};
```

- `Deserialize(path)`: `glz::read_file_jsonc(SceneFileData, path)` → 创建 Scene/Camera/GameObjects
- `Serialize(path)`: 遍历 GameObjects → `glz::write_file_json(SceneFileData, path)`

## 组件注册方式

每个组件的 `.cpp` 文件中添加静态初始化：

```cpp
// SpriteRenderer.cpp
namespace {
    bool registered = []() {
        ComponentRegistry::Get().Register<SpriteRenderer>("SpriteRenderer");
        return true;
    }();
}
```

无需手动维护注册列表。

## 数学类型处理

GLM 类型（vec3, quat 等）通过 `std::array` 在 JSON 中表示，Data struct 内做转换。

## 实施顺序

1. 新建 `ComponentRegistry` (h/cpp)
2. `Component.h` — 加虚方法
3. `Transform` — 加 Data/GetData/SetData
4. `GameObject` — 加序列化方法
5. `Scene` — 重写 Deserialize，加 Serialize
6. `SpriteRenderer` — 加 Data 作为示例
7. 更新场景文件到新格式
8. 更新 Prisma2DApp 适配
