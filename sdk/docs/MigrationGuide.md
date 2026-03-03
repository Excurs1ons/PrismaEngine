# PrismaEngine 迁移指南

## 从其他引擎迁移到 PrismaEngine

本指南帮助开发者从其他游戏引擎迁移到 PrismaEngine。

## 目录

- [从 Unity 迁移](#从-unity-迁移)
- [从 Unreal Engine 迁移](#从-unreal-engine-迁移)
- [从 Godot 迁移](#从-godot-迁移)
- [从自定义引擎迁移](#从自定义引擎迁移)

---

## 从 Unity 迁移

### 核心概念映射

| Unity | PrismaEngine | 说明 |
|-------|--------------|------|
| GameObject | GameObject | 游戏对象 |
| Component | Component | 组件系统 |
| MonoBehaviour | ScriptComponent | 脚本组件 |
| Transform | Transform | 变换 |
| Rigidbody | PhysicsComponent | 刚体 |
| Collider | Collider | 碰撞体 |
| Material | Material | 材质 |
| Shader | Shader | 着色器 |
| Prefab | Asset | 预制体（资源） |

### 代码示例对比

**Unity (C#)**:
```csharp
using UnityEngine;

public class PlayerController : MonoBehaviour {
    public float speed = 5.0f;

    void Update() {
        float horizontal = Input.GetAxis("Horizontal");
        float vertical = Input.GetAxis("Vertical");

        Vector3 movement = new Vector3(horizontal, 0, vertical);
        transform.Translate(movement * speed * Time.deltaTime);
    }
}
```

**PrismaEngine (C++)**:
```cpp
#include <PrismaEngine/PrismaEngine.h>

class PlayerController : public PrismaEngine::ScriptComponent {
public:
    float speed = 5.0f;

    void Update(float deltaTime) override {
        auto* input = PrismaEngine::Engine::GetInputManager();

        float horizontal = 0.0f;
        float vertical = 0.0f;

        if (input->IsKeyDown(KeyCode::A)) horizontal -= 1.0f;
        if (input->IsKeyDown(KeyCode::D)) horizontal += 1.0f;
        if (input->IsKeyDown(KeyCode::W)) vertical += 1.0f;
        if (input->IsKeyDown(KeyCode::S)) vertical -= 1.0f;

        glm::vec3 movement(horizontal, 0.0f, vertical);
        GetTransform()->Translate(movement * speed * deltaTime);
    }
};
```

### 关键差异

1. **语言**: C# → C++
2. **输入系统**: Input.GetAxis → IsKeyDown
3. **向量**: Vector3 → glm::vec3
4. **组件**: 继承 MonoBehaviour → 继承 ScriptComponent
5. **生命周期**: Update() → Update(float deltaTime)

---

## 从 Unreal Engine 迁移

### 核心概念映射

| Unreal Engine | PrismaEngine | 说明 |
|---------------|--------------|------|
| Actor | GameObject | 演员 |
| Component | Component | 组件 |
| AActor | GameObject | 游戏对象基类 |
| UActorComponent | Component | 组件基类 |
| USceneComponent | TransformComponent | 场景组件 |
| UPrimitiveComponent | MeshComponent | 网格组件 |
| Blueprint | ScriptComponent | 蓝图/脚本 |

### 代码示例对比

**Unreal Engine (C++)**:
```cpp
#include "GameFramework/Actor.h"

class AMyActor : public AActor {
public:
    UPROPERTY(EditAnywhere)
    float Speed = 5.0f;

    virtual void Tick(float DeltaTime) override {
        Super::Tick(DeltaTime);

        FVector Movement = FVector::ForwardVector * Speed * DeltaTime;
        AddActorLocalOffset(Movement);
    }
};
```

**PrismaEngine (C++)**:
```cpp
#include <PrismaEngine/PrismaEngine.h>

class MyActor : public PrismaEngine::GameObject {
public:
    float speed = 5.0f;

    void Update(float deltaTime) override {
        glm::vec3 movement(0.0f, 0.0f, speed * deltaTime);
        GetTransform()->Translate(movement);
    }
};
```

### 关键差异

1. **宏**: UPROPERTY → 直接成员变量
2. **类型**: FVector → glm::vec3
3. **命名**: AActor 前缀 → 无前缀
4. **反射**: 内置反射系统 → 无反射（C++限制）
5. **垃圾回收**: 自动 → 智能指针

---

## 从 Godot 迁移

### 核心概念映射

| Godot | PrismaEngine | 说明 |
|-------|--------------|------|
| Node | GameObject | 节点 |
| Resource | Asset | 资源 |
| Scene | World/Level | 场景 |
| Script | ScriptComponent | 脚本 |
| GDScript | C++ | 脚本语言 |

### 代码示例对比

**Godot (GDScript)**:
```gdscript
extends Node

export var speed = 5.0

func _process(delta):
    var input_vector = Vector2()
    if Input.is_action_pressed("ui_up"):
        input_vector.y -= 1
    if Input.is_action_pressed("ui_down"):
        input_vector.y += 1

    translate(input_vector * speed * delta)
```

**PrismaEngine (C++)**:
```cpp
#include <PrismaEngine/PrismaEngine.h>

class MyNode : public PrismaEngine::GameObject {
public:
    float speed = 5.0f;

    void Update(float deltaTime) override {
        auto* input = PrismaEngine::Engine::GetInputManager();

        glm::vec2 inputVector(0.0f);
        if (input->IsKeyDown(KeyCode::Up)) inputVector.y -= 1.0f;
        if (input->IsKeyDown(KeyCode::Down)) inputVector.y += 1.0f;

        glm::vec3 movement(inputVector.x, 0.0f, inputVector.y);
        GetTransform()->Translate(movement * speed * deltaTime);
    }
};
```

### 关键差异

1. **语言**: GDScript → C++
2. **导出**: export → 直接公开成员
3. **输入**: Input.is_action_pressed → IsKeyDown
4. **类型**: Vector2 → glm::vec2
5. **脚本**: _process → Update

---

## 从自定义引擎迁移

### 架构调整

如果你的自定义引擎已经有一定基础，迁移主要涉及：

1. **渲染后端**:
   - 保留你的渲染代码作为 PrismaEngine 的适配器
   - 或使用 PrismaEngine 的 Vulkan/DX12 后端

2. **资源管理**:
   - 将资源加载代码迁移到 AssetManager
   - 使用 PrismaEngine 的序列化系统

3. **ECS 系统**:
   - PrismaEngine 支持 ECS，可映射现有组件

4. **输入系统**:
   - 统一到 PrismaEngine::InputManager

### 迁移步骤

1. **集成 PrismaEngine**
   ```cmake
   find_package(PrismaEngine REQUIRED)
   target_link_libraries(YourGame PRIVATE PrismaEngine::Engine)
   ```

2. **保留现有代码**
   - 逐步迁移，不必一次性重写
   - 可以让两个系统共存

3. **迁移渲染**
   - 使用 PrismaEngine 的渲染设备
   - 或创建自定义适配器

4. **迁移资源**
   - 转换到 PrismaEngine 资源格式
   - 或使用自定义加载器

---

## 最佳实践

1. **渐进式迁移**:
   - 不要一次性重写所有代码
   - 逐模块迁移

2. **保持抽象**:
   - 创建包装层隔离引擎差异
   - 便于未来调整

3. **性能优化**:
   - 迁移后进行性能分析
   - 利用 PrismaEngine 的多线程能力

4. **测试**:
   - 每个迁移阶段都要充分测试
   - 确保功能一致性

---

## 获取帮助

- [API 参考](APIReference.md)
- [快速入门](QuickStart.md)
- [GitHub Issues](https://github.com/Excurs1ons/PrismaEngine/issues)
- [Discord 社区](https://discord.gg/prismaengine) (即将上线)
