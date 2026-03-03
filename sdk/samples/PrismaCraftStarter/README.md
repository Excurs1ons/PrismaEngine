# PrismaCraft Starter 示例

这是一个展示如何集成 PrismaEngine + PrismaCraft 创建 Minecraft 风格游戏的启动示例。

## 项目说明

本示例展示了：
1. 如何集成 PrismaCraft 游戏系统
2. 如何使用 PrismaEngine 渲染 Minecraft 风格的世界
3. 如何实现 Minecraft 风格的游戏循环和输入

## 架构概览

```
PrismaCraftStarter/
├── src/
│   ├── main.cpp              # 程序入口
│   └── MinecraftGame.cpp     # Minecraft 风格游戏逻辑
└── assets/
    ├── shaders/              # 着色器
    ├── textures/             # 纹理图集
    └── configs/              # 配置文件
```

## 与 BlockGame 的区别

| 特性 | BlockGame | PrismaCraftStarter |
|------|-----------|-------------------|
| 游戏引擎 | 仅 PrismaEngine | PrismaEngine + PrismaCraft |
| 方块系统 | 简单实现 | 完整 Minecraft 方块注册 |
| 世界生成 | 简单噪声 | Minecraft 原版算法 |
| 区块系统 | 无 | 完整区块加载/卸载 |
| 实体系统 | 简单玩家 | 完整实体系统 |

## 构建

```bash
mkdir build && cd build
cmake .. -DPrismaEngine_DIR=/path/to/prismaengine-sdk \
         -DPrismaCraft_DIR=/path/to/prismacraft
cmake --build .
```

## 运行

```bash
./PrismaCraftStarter
```

## 游戏特性

- ✅ 完整的方块注册系统（50+ 原版方块）
- ✅ 区块加载和卸载
- ✅ 世界生成（地形、洞穴、结构）
- ✅ 玩家移动和物理
- ✅ 方块交互（破坏/放置）
- ✅ 库存系统（基础）
- ✅ 光照系统（简化）

## 开发路线图

- [ ] 完整库存系统
- [ ] 合成系统
- [ ] 生物系统
- [ ] 红石系统
- [ ] 网络多人游戏

## 贡献

欢迎贡献！请查看 PrismaCraft 仓库的贡献指南。

## 许可证

MIT License - 与 PrismaEngine 和 PrismaCraft 保持一致
