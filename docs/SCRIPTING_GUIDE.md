# PrismaEngine 脚本系统指南

## 概述

PrismaEngine 使用 Mono 运行时提供 C# 脚本支持，类似于 Unity 的 MonoBehaviour 系统。

## 特性

- **C# 脚本**: 使用熟悉的 C# 语言编写游戏逻辑
- **MonoBehaviour**: 类似 Unity 的组件系统
- **热重载**: 支持运行时重新加载脚本（开发中）
- **性能**: JIT 编译，高性能脚本执行
- **API**: 提供 Unity 风格的 API

## 核心组件

### MonoBehaviour

所有脚本都应继承自 MonoBehaviour：

```csharp
using PrismaEngine;

public class MyScript : MonoBehaviour
{
    void Start() {
        // 初始化逻辑
    }

    void Update() {
        // 每帧更新逻辑
    }
}
```

### 可用的生命周期方法

- `Awake()`: 对象创建时调用
- `Start()`: 第一帧之前调用
- `Update()`: 每帧调用
- `FixedUpdate()`: 固定时间间隔调用
- `LateUpdate()`: Update后调用
- `OnEnable()`: 启用时调用
- `OnDisable()`: 禁用时调用
- `OnDestroy()`: 销毁时调用

## 常用API

### Transform 组件

```csharp
// 位置
transform.Position = new Vector3(0, 1, 0);
Vector3 pos = transform.Position;

// 旋转
transform.Rotation = Quaternion.Euler(0, 45, 0);

// 缩放
transform.Scale = new Vector3(1, 1, 1);

// 方向向量
Vector3 forward = transform.Forward;
Vector3 right = transform.Right;
Vector3 up = transform.Up;
```

### 输入系统

```csharp
// 键盘
if (Input.GetKey(KeyCode.W)) { }
if (Input.GetKeyDown(KeyCode.Space)) { }
if (Input.GetKeyUp(KeyCode.Escape)) { }

// 鼠标
Vector3 mousePos = Input.mousePosition;
bool leftButton = Input.GetMouseButton(0);
```

### 时间系统

```csharp
float deltaTime = Time.deltaTime;
float time = Time.time;
```

### 数学库

```csharp
// Vector3
Vector3 a = new Vector3(1, 2, 3);
Vector3 b = new Vector3(4, 5, 6);
Vector3 sum = a + b;
float distance = Vector3.Distance(a, b);

// Quaternion
Quaternion rotation = Quaternion.Euler(0, 45, 0);
Vector3 rotated = rotation * Vector3.forward;

// Mathf
float clamped = Mathf.Clamp(value, 0, 1);
float lerped = Mathf.Lerp(a, b, t);
```

### 调试

```csharp
Debug.Log("Hello World");
Debug.LogWarning("Warning");
Debug.LogError("Error");
```

## 示例脚本

### 简单移动

```csharp
using PrismaEngine;

public class SimpleMovement : MonoBehaviour
{
    public float speed = 5.0f;

    void Update()
    {
        float horizontal = Input.GetKey(KeyCode.A) ? -1 : (Input.GetKey(KeyCode.D) ? 1 : 0);
        float vertical = Input.GetKey(KeyCode.S) ? -1 : (Input.GetKey(KeyCode.W) ? 1 : 0);

        Vector3 movement = new Vector3(horizontal, 0, vertical) * speed * Time.deltaTime;
        transform.Position += movement;
    }
}
```

### 旋转物体

```csharp
using PrismaEngine;

public class Rotator : MonoBehaviour
{
    public float rotationSpeed = 90.0f;

    void Update()
    {
        transform.Rotation *= Quaternion.Euler(0, rotationSpeed * Time.deltaTime, 0);
    }
}
```

### 弹跳

```csharp
using PrismaEngine;

public class Bouncer : MonoBehaviour
{
    public float bounceHeight = 2.0f;
    public float bounceSpeed = 2.0f;
    private float startY;

    void Start()
    {
        startY = transform.Position.y;
    }

    void Update()
    {
        float newY = startY + Mathf.Abs(Mathf.Sin(Time.time * bounceSpeed)) * bounceHeight;
        transform.Position = new Vector3(transform.Position.x, newY, transform.Position.z);
    }
}
```

## 最佳实践

1. **缓存组件引用**: 在 Start 或 Awake 中获取组件引用，避免每帧查找

```csharp
private Rigidbody rb;

void Start()
{
    rb = GetComponent<Rigidbody>();
}

void Update()
{
    rb.AddForce(Vector3.forward);
}
```

2. **使用 Time.deltaTime**: 使移动与帧率无关

```csharp
transform.Position += Vector3.forward * speed * Time.deltaTime;
```

3. **避免在 Update 中分配内存**: 减少垃圾回收压力

4. **使用对象池**: 对于频繁创建销毁的对象

5. **合理使用 FixedUpdate**: 物理相关逻辑放在 FixedUpdate

## 编译和部署

1. 将 C# 脚本放在 `scripts/` 目录
2. 使用 Mono 编译器编译成 DLL：
   ```bash
   mcs -target:library -out:scripts/Game.dll scripts/*.cs
   ```
3. 在游戏初始化时加载程序集

## 注意事项

- 当前版本使用动态链接 Mono，需要系统安装 Mono 运行时
- 热重载功能正在开发中
- 某些高级功能（如协程）尚未实现
- 性能仍在优化中

## 下一步计划

- [ ] 协程（Coroutine）支持
- [ ] 动画系统集成
- [ ] UI事件系统
- [ ] 序列化支持
- [ ] Visual Studio Code 编辑器扩展