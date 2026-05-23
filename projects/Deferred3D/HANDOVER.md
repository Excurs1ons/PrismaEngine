# Deferred3D 交接文档

## 当前管线流程

```
GBuffer Pass
  → position (RGBA16F), normal (RGBA16F), albedo (RGBA8), emissive (RGBA16F), depth (D32F)

Lighting Pass (全屏三角形)
  → 读取 GBuffer 4 纹理 + push_constant (ambient/lightDir/lightColor)
  → 自发光 emissiveSelf (来自 GBuffer)
  → emissive 面光源直射 (硬编码在 shader)
  → 写入 lightingOutput (RGBA16F)

SSGI Pass (全屏三角形)
  → 读取 GBuffer 5 纹理 + lightingResult (binding 5)
  → 半球 ray marching 反弹采样
  → 写入 ssgiOutput (RGBA16F)

Composite Pass (全屏三角形)
  → 读取 lightingOutput + ssgiOutput
  → 相加输出到 swapchain
```

## 已知问题

### 1. ✅ Ambient Light 不生效 — 已修复
- **根因**：`deferred_lighting.frag` 通过 UBO binding 4 读取 `ub.ambient`，但 C++ 端从未创建/绑定 UBO。同时 C++ 通过 PushConstants 推送数据，但 shader 未声明 `layout(push_constant)`，数据路径完全断开。
- **修复**：shader 改为 `layout(push_constant) uniform PushConstants { vec4 ambient; vec4 lightDir; vec4 lightColor; } pc;`，C++ 端统一 PushConstants 结构体与之对齐。
- **状态**：2026-05-24 已修复并验证编译通过。

### 2. 重复光照 Pass — 已修复
- SSGI pass 之后存在第二个光照 pass 重新渲染到 `m_lightingOutput`，覆盖第一次光照结果。虽然因 UBO 未绑导致两次输出相同（仅有硬编码 emissive），但数据流不一致且多余。
- **修复**：删除第二个光照 pass，管线精简为 GBuffer → Lighting → SSGI → Composite。

### 3. 硬编码 emissive 面光源
- `deferred_lighting.frag` 37-61 行硬编码了 Cornell Box 天花板 Light 面参数
- 光源位置 (0, 0.995, 0)，半尺寸 0.3，emissive (10,10,10)
- 需要改为从场景数据或 push_constant 读取

### 4. SSGI 参数
- 当前值：16 rays, intensity 0.6, maxDist 5.0, stepSize 0.1
- 见 `DeferredPipelineAdapter.cpp` SSGI pass 中的 `pc.params`

## 待实现

### 高优先级
1. **去除硬编码** — emissive 面光源参数从场景数据读取（或保持硬编码但增加可配置性）
2. **SSGI totalWeight 归一化修复** — `ssgi.frag:114-116` 中 `totalWeight` 已累加但未用于归一化，应改为 `indirect = indirect * intensity / totalWeight`

### 中优先级
3. **Temporal SSGI** — 累加多帧降噪

### 低优先级
4. **LightComponent 的 Ambient 类型** — 已添加类型枚举和序列化，`Scene::GetLights()` 已支持 `IsEnabled()` 过滤

## 场景 JSON 结构

```json
AmbientLight:   type=Ambient, color=[1,1,1], intensity=1, enabled=true
DirectionalLight: type=Directional, color=[1,1,1], intensity=3, enabled=false
Light (发光面): emissive=[10,10,10], 位置(0,0.995,0), 0.6x0.6
```

## 文件清单

| 文件 | 说明 |
|------|------|
| `shaders/deferred_lighting.frag` | Lighting pass，硬编码 emissive 直射 + push_constant ambient/dir |
| `shaders/ssgi.frag` | SSGI bounce，ray marching，正常工作 |
| `shaders/deferred_composite.frag` | 合成 lighting+ssgi，正常工作 |
| `shaders/deferred_fullscreen.vert` | 全屏三角形顶点着色器，共用 |
| `shaders/gbuffer.vert/frag` | GBuffer 写入，含 emissive push constant |
| `src/DeferredPipelineAdapter.h` | 适配器声明（不再需要 m_lightingUBO） |
| `src/DeferredPipelineAdapter.cpp` | 管线执行逻辑 |
