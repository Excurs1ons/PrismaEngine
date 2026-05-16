# Prisma WebUI 编辑器

## 概述

Prisma WebUI 编辑器是一个内置的浏览器访问编辑器，通过 `WebUIEditor` 类实现。它嵌入了一个 HTTP 服务器 (cpp-httplib)，提供完整的编辑器 UI，无需任何前端框架依赖。

### 核心特性

- **零外部依赖** — 内嵌 cpp-httplib (header-only)，原生 HTML5/CSS/JS，无 Node.js/npm
- **实时视口** — Scene Viewport (1280×720 RGBA) 和 Game Viewport 双通道帧缓冲流
- **完整编辑器面板** — Hierarchy、Inspector、Console、Asset Browser
- **传输无关后端** — `EditorService` 提供统一逻辑，WebUI 和未来 Electron IPC 共享
- **共享内存帧缓冲** — `EditorSharedMemory` 零拷贝视口数据传输
- **响应式布局** — 桌面三栏 / 移动端单栏自适应

## 架构

```
Editor
├── WebUIEditor (HTTP 服务器)
│   ├── GET  /                     → 主页面 HTML (内嵌完整编辑器)
│   └── POST /api/v1/(.*)          → API 路由
│       ├── hierarchy/get          → EditorService::GetHierarchy()
│       ├── entity/get             → EditorService::GetEntity()
│       ├── entity/update          → EditorService::UpdateEntity()
│       ├── engine/status          → EditorService::GetStatus()
│       ├── console/get            → 引擎日志
│       └── assets/list            → AssetDatabase
└── EditorService (传输无关)
    └── EditorSharedMemory → Scene View / Game View 帧缓冲
```

## 启动

```bash
# 启动编辑器并开启 WebUI (默认端口 8080)
./bin/PrismaEditor --webui

# 指定端口
./bin/PrismaEditor --webui --webui-port=9090
```

然后浏览器访问 `http://localhost:8080` (或指定端口)。

## 功能面板

### Scene Viewport (场景视口)

- 实时显示 1280×720 RGBA 帧缓冲，60fps 刷新
- 通过 Canvas 2D 渲染 (requestAnimationFrame 轮询)
- 鼠标拖拽平移场景 (Pan)
- 滚轮缩放 (Zoom)
- 反浏览器缓存 (时间戳后缀 `?t=Date.now()`)

### Hierarchy (层级面板)

- 显示场景中所有实体的列表
- 点击选中实体，高亮显示
- 支持 `New Entity` 按钮创建实体

### Inspector (检查器)

- 选中实体后显示 Transform 属性
- 实时编辑 Position (X/Y/Z)
- 变更通过 `entity/update` API 应用

### Console (控制台)

- 实时显示引擎日志输出
- 自动滚动，每 2 秒刷新

### Asset Browser (资源浏览器)

- 浏览 AssetDatabase 中的所有资源
- 按路径列出

## API 参考

### GET /

返回编辑器主页面 HTML。包含完整的编辑器 UI 实现 (~150 行内联 HTML/CSS/JS)。

### GET /api/v1/viewport/scene

返回 1280×720×4 字节的 RGBA 帧缓冲。

### POST /api/v1/:action

JSON 请求/响应 API：

| 动作 | 请求参数 | 响应 | 描述 |
|------|----------|------|------|
| `hierarchy/get` | `{}` | `{entities: [{id, name}]}` | 场景层级 |
| `entity/get` | `{id}` | `{name, components}` | 实体详情 |
| `entity/update` | `{id, data}` | `{success}` | 更新实体 |
| `entity/create` | `{}` | `{entity_id, name}` | 创建实体 |
| `engine/status` | `{}` | `{fps, gpu, scene, objects}` | 引擎状态 |
| `console/get` | `{}` | `{logs}` | 控制台日志 |
| `assets/list` | `{}` | `{assets}` | 资源列表 |
| `viewport/input` | `{type, dx, dy}` | `{success}` | 视口输入 |

## 技术栈

| 组件 | 技术 | 位置 |
|------|------|------|
| HTTP 服务器 | cpp-httplib (header-only) | `src/editor/core/httplib.h` |
| WebUI 控制器 | `WebUIEditor` | `src/editor/core/WebUIEditor.h/.cpp` |
| 服务后端 | `EditorService` | `src/editor/core/EditorService.h/.cpp` |
| 帧缓冲 | `EditorSharedMemory` | `src/editor/core/EditorSharedMemory.h` |
| 序列化 | glaze (glz::json_t) | 已有依赖 |
| 前端 | 原生 HTML5 + CSS3 Grid + Canvas 2D | 内嵌在 WebUIEditor.cpp |

## 前端结构

编辑器前端完全内嵌在 C++ 源文件中，通过字符串返回给浏览器：

```
HTML
├── <style>        — CSS 变量、Grid 布局、响应式断点
├── <div.nav>      — 顶栏 (FPS/场景信息、操作按钮)
├── <div.layout>
│   ├── Hierarchy — 左侧 240px 实体列表
│   ├── Center    — Viewport Canvas + Console + Assets
│   └── Inspector — 右侧 300px 属性编辑
└── <script>
    ├── api()      — 通用 fetch 封装
    ├── stream()   — Canvas 帧循环 (requestAnimationFrame)
    ├── sync()     — 数据轮询 (setInterval 2s)
    ├── select()   — 选中实体 → 更新 Inspector
    └── updatePos() — Transform 编辑 → API 写回
```

## 已知限制

- 视口渲染目前为软件模拟 (Unity 蓝色背景 + 白色方块标记物体位置)
  - 可通过 `InternalDoRender()` 接入真实 GPU 帧缓冲 (当前已预留双通道 SHM)
- 仅支持单个客户端连接
- Inspector 仅支持 Transform 组件编辑
- 无 WebSocket 推送，使用轮询模式 (2s 间隔)

## 相关代码

- `src/editor/core/WebUIEditor.h` — WebUI 服务器声明
- `src/editor/core/WebUIEditor.cpp` — HTTP 路由 + 前端页面实现
- `src/editor/core/EditorService.h` — 传输无关编辑器服务接口
- `src/editor/core/EditorService.cpp` — 业务逻辑实现
- `src/editor/core/EditorSharedMemory.h` — 共享内存帧缓冲
- `src/editor/core/Editor.h` — Editor 类 (持有 WebUIEditor 实例)
- `src/editor/core/Editor.cpp` — `--webui` CLI 解析 + 启动
