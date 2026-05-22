# 场景系统重构总结

## 概述

将场景序列化/反序列化能力完整化，使 Component/GameObject/Transform/Scene 支持 JSONC 格式的序列化。同时将 Template2DApp 中的引擎级样板代码上提到引擎层。

## Glaze 升级

- `cmake/DependencyVersions.cmake`: `v4.2.2` → `v7.5.0`
- 修复 MSVC `std::strong_equality` 兼容问题（v7.5.0 已完全移除该类型引用）
- 主要 API: `glz::read_file_json`, `glz::read_file_jsonc`, `glz::write_file_json`, `glz::format_error`, `glz::generic`

## 引擎功能增强

### Engine.h/.cpp
- `FrameStats::FPS` — 引擎内建 FPS 统计
- `GetGPUName()` — GPU 名称快捷获取
- `GetFPS()` — 帧率查询
- 窗口创建后自动回写实际尺寸到 app spec
- `WindowResizeEvent` 自动更新场景主相机视口
- `OnInitialize()` 后自动同步相机到窗口尺寸（新加）

### ICamera.h
- 新增纯虚方法 `SetViewport(uint32_t width, uint32_t height)`
- 相机自行处理视口变化，Engine 不再直接操作投影矩阵

### OrthographicCamera.h
- 实现 `SetViewport(w, h)` → `SetProjection(0, w, 0, h)`（左下角原点坐标系）

### CMakeLists.txt
- 添加 `core/ComponentRegistry.cpp/.h` 到构建

## 新增文件

### ComponentRegistry（core/）
类型注册表 + 序列化回调：

```
ComponentRegistry::Get()
.Register<T>("TypeName")              — 基础注册（工厂）
.RegisterSerializable("Name", ser, deser) — 可序列化注册
.Create("TypeName")                   — 按类型名创建组件
.GetTypeName(component)               — 反向查询类型名
.SerializeComponent(comp)             — 获取组件数据 JSON
.DeserializeComponent(comp, type, json) — 从 JSON 恢复组件数据
```

## 修改文件详情

### Component.h
- 新增虚方法 `virtual const char* GetComponentTypeName() const`（默认返回 nullptr）

### Transform.h/.cpp
- 新增 `Data` struct: `{position[3], rotation[4](xyzw), scale[3]}`
- 新增 `GetData()`, `SetData()` 方法
- `GetComponentTypeName()` → `"Transform"`

### GameObject.h/.cpp
- 新增 `ComponentEntry` struct: `{type, dataJson}`
- 新增 `Data` struct: `{name, transform, components[]}`
- 新增 `GetData()`, `SetData()` 方法
- 新增 `GetComponents()` const 访问器
- 序列化流程: Component → Registry::SerializeComponent → JSON string
- 反序列化流程: JSON string → Registry::DeserializeComponent → Component::SetData

### Scene.h/.cpp
- 新增 `SceneFileData` 顶层结构: `{name, camera.projection[], gameObjects[]}`
- 重写 `Deserialize()`: 使用 `read_file_jsonc`，创建 GameObjects 并通过 `SetData()` 恢复
- 新增 `Serialize()`: 收集场景数据并通过 `write_file_json` 写出
- 移除旧的 SpriteConfig/CameraConfig 专有结构

### SpriteRenderer.h/.cpp
- 新增 `Data` struct: `{color[4], position[2], size[2], rotation}`
- 新增 `GetData()`, `SetData()` 方法
- `GetComponentTypeName()` → `"SpriteRenderer"`
- 静态注册: `ComponentRegistry::Register<SpriteRenderer> + RegisterSerializable`

### Template2DApp.h/.cpp

**移除：**
- `m_CameraComponent`（相机来自 SceneManager）
- `m_gpuName`, 所有 FPS 统计成员（来自 Engine）
- `LoadScene()` 方法
- `SpriteConfig`, `SceneFileSprites` 等 Glaze 专有结构
- `CameraComponent.h` include

**新增：**
- `m_sceneSprites` — 场景 SpriteRenderer 缓存
- 使用 `SceneManager::LoadFromFile()` 加载场景
- 相机来自 `SceneManager::GetCurrentScene()->GetMainCamera()`
- FPS/GPU 来自 `Engine::GetFPS()/GetGPUName()`
- 保留 20 个随机 TestSprite（demo 专有逻辑）

## 场景文件格式

**旧格式**（`.json`）:
```json
{
  "name": "...",
  "camera": { "projection": { "left":..., "right":..., "bottom":..., "top":... } },
  "settings": { "clearColor": [...], "showGrid": true },
  "sprites": [ { "position": [...], "size": [...], "color": [...], "rotation": 0 } ]
}
```

**新格式**（`.jsonc`，支持注释）:
```jsonc
{
  "name": "...",
  "camera": { "projection": [0, 1920, 0, 1080] },
  "gameObjects": [
    {
      "name": "Sprite",
      "transform": { "position": [x, y, z], "rotation": [x, y, z, w], "scale": [x, y, z] },
      "components": [
        { "type": "SpriteRenderer", "data": { "color": [...], "position": [...], "size": [...], "rotation": 0 } }
      ]
    }
  ]
}
```

## 后续改进（2026-05-10 第2轮）

### ProjectConfig.h — 项目配置引擎化
- 新建 `src/engine/app/ProjectConfig.h`：`WindowConfig`/`ProjectConfig` struct + `PresentMode`/`WindowConfig`/`ProjectConfig` 的 Glaze meta
- `Engine::Run()` 自动读取 project.json，填充 `ApplicationSpecification` + 注册资产路径
- `Engine::Initialize()` 移除了重复的 project.json assets 读取
- `Template2DApp::LoadSpecification()` 删除
- `Template2DApp` 构造函数简化为 aggregate init: `Application({"Template2D", "scenes/2d_test.jsonc", ...})`

### ComponentEntry — glz::generic 修复 JSON 读取
- `dataJson` (string) → `data` (`glz::generic`)
- 修复 `unknown_key` 错误：JSONC 中 `"data": {...}` 对象可直接读入 `glz::generic`
- 序列化: `reg.SerializeComponent` → string → `glz::read_json` → `glz::generic`
- 反序列化: `glz::generic` → `.dump()` → string → `reg.DeserializeComponent`

### SceneManager — AssetManager 路径解析
- `LoadFromFile()` 使用 `Engine::Get().GetAssetManager()->FindResource()` 解析相对路径
- 修复 `file_open_failure` 错误

### Engine — resize 时更新 spec 窗口尺寸
- `WindowResizeEvent` 中自动更新 `m_CurrentApp->GetSpecification().Width/Height`
- `Template2DApp::OnEvent` 不再需要处理 resize

### 相机移动（方向键 + WASD）
- `Template2DApp` 新增 `m_moveUp/Down/Left/Right` 标志 + `m_cameraMoveSpeed` (600px/s)
- `OnUpdate`: 根据按键状态计算移动向量，通过 `dynamic_cast<OrthographicCamera>` 调用 `SetPosition`
- `OnEvent`: 跟踪 `KeyPressedEvent`/`KeyReleasedEvent` 设置标志

### HUD 覆盖层（屏幕固定）
- 渲染分两层但共享同一个 `BeginScene/EndScene`（避免命令队列被清空）
- 场景层: 网格、精灵、坐标标注，受相机变换影响
- HUD 层: 诊断信息用 `screenPos + cameraPos` 做偏移补偿，在屏幕上位置固定

## 设计模式

### 组件序列化
```
Component
  ├── GetComponentTypeName() → "SpriteRenderer"
  ├── struct Data { ... }    ← Glaze 反射
  ├── GetData() → Data       ← 导出
  └── SetData(Data)          ← 恢复
```

### 注册 + 序列化回调
```
SpriteRenderer.cpp:
  reg.Register<SpriteRenderer>("SpriteRenderer")           // 工厂
  reg.RegisterSerializable("SpriteRenderer",                // 序列化
    [](comp) → write_json(static_cast<const T&>(comp).GetData()),
    [](comp, json) → read_json(data, json); comp.SetData(data))
```

### 场景 I/O 流程
```
写: Scene → GameObjects → GetData() → Registry::SerializeComponent → JSON string → SceneFileData → write_json
读: read_file_jsonc → SceneFileData → for each: Create GameObject → SetData() → for each component: Registry::Create + Registry::DeserializeComponent
```
