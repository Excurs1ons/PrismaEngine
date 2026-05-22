# Template2D EXE Target Design

## 目标
将 Template2D 从纯 SHARED 库改造为同时支持 Standalone EXE + DLL Plugin 双模式。

## 架构

```
Template2D_src (OBJECT 库)
  ├── Template2DApp.cpp / .h   ← 共享游戏逻辑
  └── (无入口函数)

Template2D (EXECUTABLE, WIN32)
  ├── main.cpp              ← EXE 入口，使用 RunApplication()
  ├── $<TARGET_OBJECTS:Template2D_src>
  └── 链接 Engine PRIVATE

Template2D_plugin (SHARED)
  ├── CreateApplication.cpp  ← 插件入口
  ├── $<TARGET_OBJECTS:Template2D_src>
  └── 链接 Engine PRIVATE
```

## 变更清单

| 文件 | 变更 |
|------|------|
| `projects/Template2D/CMakeLists.txt` | 分拆 OBJECT + EXE + SHARED 三目标，更新 VS Debug 配置 |
| `projects/Template2D/src/Template2DExport.h` | 删除（EXE 不需要导出宏） |
| `projects/Template2D/src/Template2DApp.h` | 去掉 TEMPLATE2D_API 宏 |
| `projects/Template2D/src/main.cpp` | 已正确使用 RunApplication()，仅小调整 |
| `projects/Template2D/src/CreateApplication.cpp` | 保留，用于插件模式 |
| `projects/CMakeLists.txt` | 无需大改 |

## VS Debug 体验

- F5 可直接启动 `Template2D.exe`
- 插件目标用于 Launcher 加载路径
