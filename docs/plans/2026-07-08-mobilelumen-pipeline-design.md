# MobileLumenPipeline 最小可渲染骨架设计

日期: 2026-07-08
状态: 已批准
分支: dev

## 背景

`MobileLumenPipeline`(`src/engine/graphic/pipelines/mobilelumen/`)当前是纯骨架:`Initialize` 仅存 device 指针,`Execute` 只跑空 swapchain render pass,导致 MLGIPipeline 项目以 MobileLumen 模式运行时黑屏。`RenderTypes.h` 已注册 `Mode3D_MobileLumen = 9`,`RenderSystem.cpp` 已集成实例化分支与 `GetRequiredRTMode` 返回 `RayQuery`。

其余本地改动(Android 插件配置化、C# 绑定 public 化、BufferType 重命名、RenderSystem RT 模式动态化、MLGIPipeline 配置切换)均已自洽完整,不在本次范围。

## 目标

最小可渲染:让 MLGIPipeline 项目以 MobileLumen 模式运行时,显示 UE3 默认场景风格的画面——**程序化物理天空盒 + 地面平面**。必须使用**正确的管线骨架**(为 MLGI 集成预留扩展点),不接 MLGI 实际功能。

## 设计决策

### 架构:独立精简骨架(方案 B)

排除方案:
- **A 组合委托 ForwardPipeline**:`Execute` 是黑盒,MLGI 无法在 opaque 前插入,骨架不正确
- **C 继承 ForwardPipeline**:成员皆 private,子类不可访问

采用:MobileLumenPipeline 自持 sub-pass 实例,编排精简 `Execute`,MLGI 预留注入点。

### 天空盒:程序化物理散射(隔离新建)

用户要求:程序化天空盒,基于物理参数,无需贴图。

调查结论:
- `SkyboxPass` 接受 `nullptr` cubemap,但 `skybox.frag` 仅简单 3 色渐变 fallback(非物理)
- 无任何 Rayleigh/Mie/大气散射实现
- 方向光(太阳)基础设施完整:`LightComponent` + `RenderContext.lights`

方案:新建隔离的 `ProceduralSkyPass` + `procedural_sky.vert/.frag`(Rayleigh+Mie 单次散射近似),**不改**共享 `skybox.frag`/`SkyboxPass`(零回归)。

## 架构

### MobileLumenPipeline 结构

直接实例化引擎层 sub-pass 类(与 ForwardPipeline 同源,非复制逻辑):
- `std::shared_ptr<DepthPrePass> m_depthPrePass`
- `std::shared_ptr<OpaquePass> m_opaquePass`
- `std::shared_ptr<ProceduralSkyPass> m_skyPass`(新)
- 预留:`MLGISystem* m_mlgi = nullptr`(将来注入,当前空)

### Execute 流程(7 步)

```
BeginSwapChainRenderPass(clearColor)   // 仅 swapchain 模式;离屏 targetTexture 跳过
UpdateGI(ctx)                          // MLGI hook:空实现,opaque 前预留
DepthPrePass                           // SetView/Proj + Execute(passContext)
OpaquePass                             // Execute(cmd, Renderer::GetCommandQueue())
ProceduralSkyPass                      // 程序化物理天空
EndSwapChainRenderPass()
```

复用 ForwardPipeline 的 `SceneData`/`PassExecutionContext`/`TextureRenderTargetProxy` 模式。不含 SSAO/TAA/Bloom/Gizmo/UI(最小)。

### MLGI 扩展点(预留)

- `virtual void UpdateGI(const RenderContext&) {}` — opaque 前调用,将来跑 ProbeUpdate+Temporal+ScreenGather
- 当前空实现,日志保留 "MLGI Phase1 骨架(预留)"

### ProceduralSkyPass

- 引擎层新 pass,复用 skybox 立方体 mesh,不依赖 cubemap 贴图
- `procedural_sky.frag`:Rayleigh + Mie 单次散射近似(Preetham 风格)
- 参数:
  - 太阳方向/强度:从 `RenderContext.lights` 方向光取(`light.direction.w < 0.5f` 为方向光)
  - Rayleigh/Mie 系数:UE 风格默认值
  - 天空色调:默认值
- 接口:`SetViewMatrix/SetProjectionMatrix + Execute(passContext)`,与 SkyboxPass 一致

### 场景内容

- 新建 `projects/MLGIPipeline/assets/scenes/default.scene.json`:地面平面 + 摄像机 + 方向光(太阳)
- 天空由管线程序化生成(无需场景天空组件),太阳方向 = 场景方向光方向
- `MLGIPipeline.jsonc` entry scene 改指向 default(保留 cornell_box 供 MLGI/PathTracing 测试)

## 数据流

```
RenderSystem::EndFrame
  -> 构造 RenderContext(device, camera, lights, clearColor, width, height, ...)
  -> MobileLumenPipeline::Execute(ctx)
       lights: 来自 Scene::GetLights(),含方向光(太阳)
       Renderer::GetCommandQueue(): 场景 MeshRenderer 自动生成的绘制命令
       ProceduralSkyPass: 从 ctx.lights 取太阳方向
```

## 范围边界

**做**:
- MobileLumenPipeline 骨架(7 步 Execute)
- ProceduralSkyPass + `procedural_sky.vert/.frag`
- MLGI hook(空实现)
- `default.scene.json`
- `MLGIPipeline.jsonc` entry scene 切换

**不做**:
- MLGI 实际功能(ProbeUpdate/Temporal/ScreenGather)
- 改 cornell_box 场景
- 改 `skybox.frag` / `SkyboxPass`
- 动其他 5 主题本地改动(Android/C#/BufferType/RenderSystem RT/MLGIPipeline 配置)

## 验证

- `Engine` target 编译通过(Windows x64 Debug,VS2022 + Vulkan)
- MLGIPipeline 项目 MobileLumen 模式运行:蓝色物理天空 + 太阳圆盘 + 地面平面,无黑屏

## 参考

- ForwardPipeline: `src/engine/graphic/pipelines/forward/ForwardPipeline.{h,cpp}`
- SkyboxRenderPass: `src/engine/graphic/pipelines/SkyboxRenderPass.{h,cpp}`
- RenderContext / IPipeline: `src/engine/graphic/interfaces/IPipeline.h`
- MLGISystem: `projects/MLGIPipeline/src/MLGISystem.{h,cpp}`
- RenderSystem 驱动: `src/engine/graphic/RenderSystem.cpp`
