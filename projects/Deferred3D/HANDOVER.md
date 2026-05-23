# Deferred3D 交接文档

## 当前管线流程

```
GBuffer Pass
  → position (RGBA16F), normal (RGBA16F), albedo (RGBA8), emissive (RGBA16F), depth (D32F)

Lighting Pass (全屏三角形)
  → 读取 GBuffer 4 纹理 + UBO (binding 4)
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

### 1. Vulkan Push Constants 不通
- `deferred_lighting.frag` 中经过测试，push constant 的 ambient、lightDir、lightColor、lightPos **全部无法读取**（返回全零）
- SSGI 的 `vp`(mat4) + `cameraPos` + `params` push constants **正常工作**
- 差异：SSGI PSO 是用 `deferred_fullscreen.vert + ssgi.frag` 创建，lighting PSO 是用 `deferred_fullscreen.vert + deferred_lighting.frag` 创建
- 原因未知，怀疑是 lighting PSO 的 pipeline layout 创建有问题，但 SSGI 同样流程正常

**当前规避方案**：
- `deferred_lighting.frag` 中的 emissive 面光源参数硬编码在 shader 里
- 场景光源（ambient/directional）计划改用 UBO binding 4 传递，但**尚未实现 UBO 的创建和绑定**
- 当前 lighting pass 的 PushConstants 调用仅传 1 个 vec4(ambient)，但仍然不通

### 2. 硬编码 emissive 面光源
- `deferred_lighting.frag` 37-61 行硬编码了 Cornell Box 天花板 Light 面参数
- 光源位置 (0, 0.995, 0)，半尺寸 0.3，emissive (10,10,10)
- 需要改为从场景数据或 UBO 读取

### 3. SSGI 参数
- 当前值：12 rays, intensity 0.6, maxDist 3.0, stepSize 0.2
- 见 `DeferredPipelineAdapter.cpp` SSGI pass 中的 `pc.params`

## 待实现

### 高优先级
1. **UBO 创建和绑定** — 在 `CreatePSOs()` 中为 lighting PSO 创建 UBO (`m_lightingUBO`)，绑定到 DS binding 4，每帧更新
2. **场景光源接入** — 从 `ctx.lights` 解析 ambient(类型3) 和 directional(类型0)，写入 UBO
3. **去除硬编码** — emissive 面光源参数从场景数据读取（或保持硬编码但增加可配置性）

### 中优先级
4. **Push Constant 问题排查** — 对比 SSGI PSO 和 lighting PSO 的 pipeline layout 创建过程，找出差异
5. **Temporal SSGI** — 累加多帧降噪

### 低优先级
6. **LightComponent 的 Ambient 类型** — 已添加类型枚举和序列化，`Scene::GetLights()` 已支持 `IsEnabled()` 过滤

## 场景 JSON 结构

```json
AmbientLight:   type=Ambient, color=[1,1,1], intensity=1, enabled=true
DirectionalLight: type=Directional, color=[1,1,1], intensity=3, enabled=false
Light (发光面): emissive=[10,10,10], 位置(0,0.995,0), 0.6x0.6
```

## 文件清单

| 文件 | 说明 |
|------|------|
| `shaders/deferred_lighting.frag` | Lighting pass，硬编码 emissive 直射，UBO binding 4 待实现 |
| `shaders/ssgi.frag` | SSGI bounce，ray marching，正常工作 |
| `shaders/deferred_composite.frag` | 合成 lighting+ssgi，正常工作 |
| `shaders/deferred_fullscreen.vert` | 全屏三角形顶点着色器，共用 |
| `shaders/gbuffer.vert/frag` | GBuffer 写入，含 emissive push constant |
| `src/DeferredPipelineAdapter.h` | 适配器声明，含 m_lightingUBO 未使用 |
| `src/DeferredPipelineAdapter.cpp` | 管线执行逻辑 |
