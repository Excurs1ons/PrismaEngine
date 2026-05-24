# Prisma2D EXE + Plugin Target 实施计划

> **For Claude:** 直接在当前会话实施。

**目标:** 将 Prisma2D 改造为 EXE (standalone) + DLL (plugin) 双模式。

**架构:** OBJECT 库共享 Prisma2DApp.cpp/h；EXE 用 main.cpp 入口 + RunApplication()；Plugin 保留 CreateApplication.cpp。

**涉及文件:**
- 删除: `projects/Prisma2D/src/Prisma2DExport.h`
- 修改: `projects/Prisma2D/src/Prisma2DApp.h` — 去掉导出宏
- 修改: `projects/Prisma2D/src/CreateApplication.cpp` — 内联导出声明
- 重写: `projects/Prisma2D/CMakeLists.txt` — OBJECT + EXE + SHARED 三目标

---

### Task 1: 清理源码文件

**Step 1.1:** 删除 `Prisma2DExport.h`

**Step 1.2:** 修改 `Prisma2DApp.h`
- 删除 `#include "Prisma2DExport.h"`
- 删除 `TEMPLATE2D_API` (class 前)

**Step 1.3:** 修改 `CreateApplication.cpp`
- 删除 `#include "Prisma2DExport.h"`
- 用内联宏替代 `TEMPLATE2D_API`

---

### Task 2: 重写 CMakeLists.txt

**结构:**
```cmake
# 共享源码变量
set(TEMPLATE2D_SOURCES src/Prisma2DApp.cpp src/Prisma2DApp.h)

# EXE target
add_executable(${PROJECT_NAME} WIN32 src/main.cpp ${TEMPLATE2D_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE Engine)

# Plugin target
add_library(${PROJECT_NAME}_plugin SHARED src/CreateApplication.cpp ${TEMPLATE2D_SOURCES})
target_link_libraries(${PROJECT_NAME}_plugin PRIVATE Engine)
target_compile_definitions(${PROJECT_NAME}_plugin PRIVATE TEMPLATE2D_PLUGIN_EXPORTS=1)
```

- VS Debug: F5 启动 Prisma2D.exe
- Plugin 编译到同目录，共享资源/C#脚本
- Post-build 复制资产到 EXE 目标目录

---

### Task 3: 编译验证

```bash
cmake --build --preset windows-x64-debug --target Prisma2D
cmake --build --preset windows-x64-debug --target Prisma2D_plugin
```
