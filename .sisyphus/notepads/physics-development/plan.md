# 物理系统开发计划
## Prisma Engine Physics System Development Plan

**创建日期**: 2026-03-09
**优先级**: 高
**当前状态**: 10% (基础框架存在)

---

## 一、当前状态分析

### 已实现 ✅

**CollisionSystem.h** (539 行，纯头文件实现)
- ✅ AABB 轴对齐包围盒（完整实现）
- ✅ Ray 射线和 RaycastHit
- ✅ AABB vs AABB 碰撞检测
- ✅ AABB vs AABB 碰撞检测（带穿透深度）
- ✅ 射线 vs AABB 检测（带法线）
- ✅ 多个 AABB 射线检测（返回最近的碰撞）
- ✅ 扫描检测（Sweep test）
- ✅ 碰撞响应（解决实体与环境的碰撞）

**PhysicsSystem** (基础框架)
- ✅ ManagerBase 继承
- ✅ WorkerThread 集成
- ✅ Initialize/Shutdown/Update 框架

### 缺失功能 ❌

1. **刚体动力学系统**
   - ❌ RigidBody 类
   - ❌ 质量、惯性张量
   - ❌ 速度、加速度、角速度
   - ❌ 力和力矩应用
   - ❌ 阻尼和摩擦

2. **物理材质系统**
   - ❌ Material 类
   - ❌ 摩擦系数
   - ❌ 恢复系数（弹性）
   - ❌ 材质碰撞表

3. **约束和关节系统**
   - ❌ Joint 类层次结构
   - ❌ 固定关节
   - ❌ 铰链关节
   - ❌ 滑动关节
   - ❌ 弹簧关节

4. **碰撞检测集成**
   - ❌ 球体 vs 球体
   - ❌ 球体 vs AABB
   - ❌ 凸包碰撞
   - ❌ 碰撞对生成

5. **空间划分**
   - ❌ BVH（包围体层次结构）
   - ❌ 网格空间划分
   - ❌ 八叉树/四叉树

6. **物理组件集成**
   - ❌ RigidBodyComponent (ECS)
   - ❌ ColliderComponent (ECS)
   - ❌ PhysicsMaterialComponent (ECS)

7. **触发器和传感器**
   - ❌ Trigger 系统
   - ❌ Overlap 检测

---

## 二、开发路线图

### Phase 1: 基础刚体系统（优先级：最高）

**目标**: 实现基本的刚体动力学模拟

**任务**:
- [ ] 1.1 创建 `RigidBody` 类
  - [ ] 基础属性：位置、旋转、缩放
  - [ ] 线性速度、角速度
  - [ ] 质量、质量倒数（倒数用于优化）
  - [ ] 惯性张量
  - [ ] 碰撞标志（静态/动态/运动学）

- [ ] 1.2 实现力和力矩应用
  - [ ] ApplyForce（世界空间力）
  - [ ] ApplyTorque（世界空间力矩）
  - [ ] ApplyImpulse（冲量）
  - [ ] ApplyForceAtPoint（在指定点施加力）

- [ ] 1.3 实现刚体更新
  - [ ] Symplectic Euler 积分器
  - [ ] 速度和位置更新
  - [ ] 阻尼（线性和角）

- [ ] 1.4 集成到 PhysicsSystem
  - [ ] 刚体管理（添加/移除/查询）
  - [ ] 物理步进（固定时间步长）
  - [ ] 与 WorkerThread 集成

**文件结构**:
```
src/engine/physics/
├── RigidBody.h               # 刚体类定义
├── RigidBody.cpp             # 刚体实现
├── PhysicsIntegrator.h        # 积分器接口
├── PhysicsIntegrator.cpp      # 积分器实现
└── RigidBodyComponent.h       # ECS 刚体组件
```

**预计工时**: 4-6 小时

---

### Phase 2: 碰撞检测扩展（优先级：高）

**目标**: 扩展碰撞检测以支持多种形状

**任务**:
- [ ] 2.1 创建 `Collider` 类层次结构
  - [ ] ICollider 接口
  - [ ] SphereCollider（球体碰撞体）
  - [ ] BoxCollider（盒碰撞体）
  - [ ] CapsuleCollider（胶囊体）
  - [ ] MeshCollider（网格碰撞体）

- [ ] 2.2 实现基本碰撞检测算法
  - [ ] Sphere vs Sphere
  - [ ] Sphere vs AABB
  - [ ] Sphere vs Box
  - [ ] Box vs Box（OBB - Oriented Bounding Box）
  - [ ] GJK 算法（用于凸包）

- [ ] 2.3 碰撞信息生成
  - [ ] Collision 接触点数据结构
  - [ ] Contact manifold 生成
  - [ ] 分离轴定理（SAT）支持

- [ ] 2.4 空间加速结构
  - [ ] 粗略阶段：AABB 笼罩
  - [ ] BroadPhase 类
  - [ ] 细致阶段：精确碰撞检测

**文件结构**:
```
src/engine/physics/
├── collider/
│   ├── ICollider.h           # 碰撞体接口
│   ├── SphereCollider.h      # 球体
│   ├── BoxCollider.h         # 盒子
│   ├── CapsuleCollider.h     # 胶囊
│   └── MeshCollider.h       # 网格
├── collision/
│   ├── BroadPhase.h         # 粗略阶段
│   ├── NarrowPhase.h        # 细致阶段
│   ├── ContactManifold.h     # 接触流形
│   └── GJK.h             # GJK 算法
└── ColliderComponent.h       # ECS 碰撞体组件
```

**预计工时**: 8-12 小时

---

### Phase 3: 碰撞响应（优先级：高）

**目标**: 实现物理碰撞响应和求解

**任务**:
- [ ] 3.1 创建 `PhysicsMaterial` 类
  - [ ] 静态摩擦系数
  - [ ] 动态摩擦系数
  - [ ] 恢复系数（弹性）
  - [ ] 材质碰撞表

- [ ] 3.2 实现碰撞求解器
  - [ ] 冲量求解器
  - [ ] 摩擦力应用
  - [ ] 穿透修正（位置更新）

- [ ] 3.3 约束求解
  - [ ] Sequential Impulses 算法
  - [ ] 多次迭代求解
  - [ ] 稳定性优化

- [ ] 3.4 触发器支持
  - [ ] Trigger 标志
  - [ ] OnTriggerEnter/Stay/Exit 回调
  - [ ] Overlap 事件

**文件结构**:
```
src/engine/physics/
├── PhysicsMaterial.h         # 物理材质
├── PhysicsMaterial.cpp
├── solver/
│   ├── CollisionSolver.h      # 碰撞求解器
│   ├── CollisionSolver.cpp
│   ├── ImpulseSolver.h      # 冲量求解器
│   ├── FrictionSolver.h      # 摩擦力求解器
│   └── ConstraintSolver.h   # 约束求解器
└── TriggerSystem.h         # 触发器系统
```

**预计工时**: 6-8 小时

---

### Phase 4: 约束和关节（优先级：中）

**目标**: 实现物理约束系统

**任务**:
- [ ] 4.1 创建 `Constraint` 类层次结构
  - [ ] IConstraint 接口
  - [ ] FixedConstraint（固定关节）
  - [ ] HingeConstraint（铰链关节）
  - [ ] SliderConstraint（滑动关节）
  - [ ] SpringConstraint（弹簧关节）

- [ ] 4.2 实现约束求解器
  - [ ] 约束雅可比矩阵
  - [ ] 拉格朗日乘数法
  - [ ] Projected Gauss-Seidel 迭代

- [ ] 4.3 集成到 PhysicsSystem
  - [ ] 约束管理
  - [ ] 每帧约束求解

**文件结构**:
```
src/engine/physics/
└── constraint/
    ├── IConstraint.h          # 约束接口
    ├── FixedConstraint.h       # 固定关节
    ├── HingeConstraint.h      # 铰链关节
    ├── SliderConstraint.h      # 滑动关节
    ├── SpringConstraint.h     # 弹簧关节
    ├── ConstraintSolver.h      # 约束求解器
    └── ConstraintSolver.cpp
```

**预计工时**: 8-10 小时

---

### Phase 5: ECS 集成（优先级：中）

**目标**: 将物理系统完全集成到 ECS

**任务**:
- [ ] 5.1 创建 ECS 物理组件
  - [ ] RigidBodyComponent
  - [ ] ColliderComponent
  - [ ] PhysicsMaterialComponent
  - [ ] TriggerComponent

- [ ] 5.2 创建物理系统（ECS ISystem）
  - [ ] PhysicsSystem（ECS 版本）
  - [ ] 与现有 PhysicsSystem 集成
  - [ ] 组件同步

- [ ] 5.3 实现查询和事件
  - [ ] 物理查询 API
  - [ ] 碰撞事件
  - [ ] 触发器事件

**预计工时**: 4-6 小时

---

### Phase 6: 测试和文档（优先级：高）

**目标**: 完善测试和文档

**任务**:
- [ ] 6.1 单元测试
  - [ ] RigidBody 测试
  - [ ] 碰撞检测测试
  - [ ] 碰撞响应测试
  - [ ] 约束求解测试

- [ ] 6.2 集成测试
  - [ ] 场景测试
  - [ ] 性能测试
  - [ ] 边界情况测试

- [ ] 6.3 文档
  - [ ] API 文档
  - [ ] 使用示例
  - [ ] 最佳实践指南

**预计工时**: 6-8 小时

---

## 三、技术规范

### 命名约定

- 类名：PascalCase (`RigidBody`, `Collider`)
- 函数名：camelCase (`applyForce`, `updatePosition`)
- 成员变量：`m_` 前缀 (`m_velocity`, `m_mass`)
- 常量：UPPER_SNAKE_CASE (`GRAVITY`, `DEFAULT_DAMPING`)

### 命名空间

```cpp
namespace PrismaEngine {
    namespace Physics {
        // 所有物理相关代码
    }
}
```

### 编码风格

- C++20 标准
- 使用 `constexpr` 进行编译时计算
- 优先使用 `glm::vec3` 而非自定义向量类
- 使用 `std::array` 和 `std::vector` 进行容器管理
- 遵循引擎现有的代码风格

### 性能考虑

- 使用平方根倒数进行距离比较（避免 sqrt）
- 使用质量倒数进行计算（避免除法）
- 使用对象池管理刚体（减少内存分配）
- 多线程支持（利用 WorkerThread）
- 空间划分加速碰撞检测

---

## 四、依赖关系

```
Phase 1 (基础刚体)
    ↓
Phase 2 (碰撞检测扩展)
    ↓
Phase 3 (碰撞响应)
    ↓
Phase 4 (约束和关节)
    ↓
Phase 5 (ECS 集成)
    ↓
Phase 6 (测试和文档)
```

---

## 五、里程碑

| 里程碑 | 目标 | 预计完成 | 状态 |
|--------|------|-----------|------|
| M1 | 基础刚体系统 | Phase 1 完成 | ⏳ |
| M2 | 完整碰撞检测 | Phase 2 完成 | ⏳ |
| M3 | 碰撞响应系统 | Phase 3 完成 | ⏳ |
| M4 | 约束系统 | Phase 4 完成 | ⏳ |
| M5 | ECS 集成 | Phase 5 完成 | ⏳ |
| M6 | 生产就绪 | Phase 6 完成 | ⏳ |

---

## 六、风险和挑战

### 技术风险

1. **GJK 算法实现复杂性**
   - 风险：算法复杂，容易出错
   - 缓解：使用参考实现，充分测试

2. **约束求解稳定性**
   - 风险：迭代求解可能不稳定
   - 缓解：多次迭代，增加稳定化步骤

3. **性能优化**
   - 风险：物理计算可能成为瓶颈
   - 缓解：空间划分、多线程、对象池

### 集成风险

1. **与 ECS 的兼容性**
   - 风险：物理组件可能破坏 ECS 设计
   - 缓解：仔细设计组件接口，保持松耦合

2. **与渲染系统的同步**
   - 风险：物理和渲染可能不同步
   - 缓解：使用固定时间步长，插值渲染

---

## 七、后续优化方向

1. **软体物理** - Cloth 和 soft body
2. **流体模拟** - SPH（平滑粒子流体动力学）
3. **GPU 加速** - CUDA/OpenCL 加速物理计算
4. **网络物理** - 网络同步和预测
5. **高级约束** - 车辆、角色控制器

---

## 八、参考资源

- **Game Physics Engine Development** - Ian Millington
- **Physics for Game Developers** - David M. Bourg
- **Real-Time Collision Detection** - Christer Ericson
- **Bullet Physics** - 开源物理引擎参考
- **Box2D** - 2D 物理引擎参考
- **PhysX** - NVIDIA 物理引擎参考
