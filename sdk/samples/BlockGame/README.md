# BlockGame 示例项目

这是一个展示如何使用 PrismaEngine 创建简单方块游戏的示例项目。

## 功能展示

- ✅ 基础游戏循环
- ✅ 输入处理（键盘、鼠标）
- ✅ 简单的世界生成
- ✅ 玩家移动和碰撞检测
- ✅ 方块放置和破坏
- ✅ 基础渲染

## 依赖

- PrismaEngine SDK
- CMake 3.20+
- C++20 编译器

## 构建

```bash
mkdir build && cd build
cmake .. -DPrismaEngine_DIR=/path/to/sdk
cmake --build .
```

## 运行

```bash
./BlockGame  # Linux/macOS
# 或
BlockGame.exe  # Windows
```

## 控制

- **W/A/S/D** - 移动
- **空格** - 跳跃
- **Shift** - 蹲下/加速
- **鼠标左键** - 破坏方块
- **鼠标右键** - 放置方块
- **1-9** - 选择方块类型
- **ESC** - 退出游戏

## 代码结构

```
src/
├── main.cpp      # 程序入口
├── Game.cpp      # 游戏主循环
├── Player.cpp    # 玩家控制
└── World.cpp     # 世界管理
```

## 学习要点

1. **游戏循环**: 如何使用固定时间步长的游戏循环
2. **输入处理**: 如何处理键盘和鼠标输入
3. **世界管理**: 如何生成和管理方块世界
4. **碰撞检测**: 如何进行简单的 AABB 碰撞检测
5. **渲染**: 如何使用 PrismaEngine 渲染 3D 场景

## 扩展建议

- 添加方块纹理
- 实现区块系统
- 添加生物/实体
- 实现库存系统
- 添加音效和音乐
