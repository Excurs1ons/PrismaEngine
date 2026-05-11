# PrismaEngine 设计哲学：规避 Unity 历史遗留问题
# Design Philosophy: Avoiding Unity's Technical Debt

PrismaEngine 在设计之初就旨在吸收现代游戏引擎的优点，并彻底规避 Unity 等传统引擎中存在的性能瓶颈和架构缺陷。

---

## 1. 跨语言调用开销 (Interop Bridge)
*   **Unity 问题**: 频繁的 C++ 与 C# 切换（如 `transform.position`）会导致严重的 Marshalling 开销。
*   **对策**: 
    *   **数据对齐 (Data Layout)**: 核心变换数据（Transform Data）尽量在内存中连续存储，C# 侧通过 `unsafe` 直接操作共享内存。
    *   **批量处理 (Batching)**: 避免逐个对象调用 API，优先使用批量更新接口。

## 2. 隐式生命周期回调 (Reflection Magic)
*   **Unity 问题**: 使用反射查找 `Update`/`Start` 方法，即使是空方法也会产生 CPU 开销。
*   **对策**: 
    *   **显式注册 (Explicit Registration)**: 脚本需显式注册所需的回调（如实现接口或通过委托缓存），引擎通过列表直接遍历函数指针。

## 3. 主线程锁定 (Main Thread Restriction)
*   **Unity 问题**: 绝大多数 API 限制在主线程访问，难以利用多核性能。
*   **对策**: 
    *   **线程安全设计 (Thread-Safe by Design)**: 核心组件设计为多线程可读。
    *   **任务并行 (Job System)**: 鼓励将逻辑拆分为无副作用的计算任务。

## 4. 垃圾回收压力 (GC Pressure)
*   **Unity 问题**: 早期 API 频繁分配临时数组/对象（如 `Input.touches`）。
*   **对策**: 
    *   **结构体优先 (Struct First)**: 数学对象（Vector, Quaternion, Rect, Color）严格使用 `struct`。
    *   **零分配 API (Zero-Allocation)**: 提供返回 `Span<T>` 或接受 `ref` 缓冲区的接口。

## 5. 坐标系与旋转逻辑 (Euler vs. Quaternions)
*   **Unity 问题**: 强推欧拉角导致万向节死锁和插值混乱。
*   **对策**: 
    *   **四元数核心 (Quaternion Core)**: 内部及 API 默认使用四元数。
    *   **右手系 (RHS)**: 统一遵循 Vulkan 标准的右手坐标系。

## 6. 全局单例的滥用 (The Singleton Trap)
*   **Unity 问题**: `Time`, `Input`, `Physics` 全局单例导致无法实现纯粹的“多世界”隔离。
*   **对策**: 
    *   **上下文注入 (Context Injection)**: 生命周期回调中显式传入 `World` 或 `Context` 实例。

## 7. 资产引用黑盒 (Implicit Serialization)
*   **Unity 问题**: `.meta` 文件与 GUID 机制极易产生 Git 冲突且不可读。
*   **对策**: 
    *   **显式路径 (Explicit Paths)**: 资源引用基于人类可读的路径（如 `res://assets/player.png`）。
    *   **纯文本场景 (JSON/YAML)**: 场景和资产配置保持纯文本化。

## 8. 组件发现低效 (Slow GetComponent)
*   **Unity 问题**: 线性遍历导致 `GetComponent` 性能极差。
*   **对策**: 
    *   **编译时索引 (Compile-time Indexing)**: 为组件类型生成唯一 ID，实现 $O(1)$ 的查表访问。

---

## 核心准则 (Core Mandates)
> **显式优于隐式，组合优于继承，并行优于串行。**
> **Explicit over Implicit, Composition over Inheritance, Parallel over Serial.**
