# PrismaEngine 开发路线图与优先级排序

**最后更新**: 2026-06-04
**排序方法**: Godot 四维需求决策框架（复杂度 / 影响范围 / 阻碍程度 / 利益相关者）

---

## 四维决策框架

| 维度 | 评分标准 |
|------|---------|
| **复杂度** (1-5) | 代码改动规模、与现有系统的耦合度、长期维护成本 |
| **影响范围** (1-5) | 功能影响的用户群体和使用场景规模 |
| **阻碍程度** (1-5) | 缺少此功能的阻碍程度，是否存在替代方案 |
| **利益相关者** (1-5) | 重度用户/商业公司权重 > 轻度用户 |

**总分 = 四维之和（满分 20），决定优先级层级**

---

## 🔴 Tier 0 — 立即执行（综合分 ≥ 16）

| 优先级 | 项 | 复杂度 | 影响范围 | 阻碍程度 | 利益相关者 | 总分 | 状态 |
|--------|-----|--------|---------|---------|-----------|------|------|
| **P0** | **Pass 实现** — OpaquePass/TransparentPass/SkyboxPass .cpp 实现 | 3 | 5 | 5 | 5 | **18** | 📋 待开始 |
| **P0** | **PBR 集成到 OpaquePass** — lit.vert/frag 接入渲染管线 | 3 | 5 | 4 | 5 | **17** | 📋 待开始 |
| **P0** | **Resource Management 收尾** — Handle<T> 系统最后 5% | 2 | 4 | 3 | 5 | **14→16** | ✅ 95% |

### P0.1 — 完成 Pass 实现

5 个核心 Pass 目前只有头文件定义（~60%），缺少 .cpp 实现：
- `OpaquePass` — PBR 不透明物体渲染
- `TransparentPass` — 透明物体渲染
- `SkyboxPass` — 天空盒渲染
- `ShadowPass` — 阴影贴图渲染
- `FinalBlitPass` — 最终输出 Blit

**阻碍**: 所有 3D 渲染管线被阻塞，无替代方案。
**涉及文件**: `src/engine/graphic/pipelines/forward/*`

### P0.2 — PBR 集成 OpaquePass

- `lit.vert` / `lit.frag` 着色器已实现
- 需要接入 OpaquePass 实现 PBR 材质渲染
- ShadowPass 架构完成待实现

### P0.3 — Resource Management 收尾

- Handle<T> 系统 + 资源池已 95%
- 收尾工作后可标记 100%

---

## 🟡 Tier 1 — 第二批（综合分 12-15）

| 优先级 | 项 | 复杂度 | 影响范围 | 阻碍程度 | 利益相关者 | 总分 | 状态 |
|--------|-----|--------|---------|---------|-----------|------|------|
| **P1** | **Scene Serialization** — ComponentRegistry + JSONC 场景 I/O | 4 | 4 | 4 | 4 | **16** | 📋 设计完成 |
| **P1** | **NEE** — Next Event Estimation（路径追踪） | 4 | 2 | 3 | 3 | **14** | 📋 设计完成 |
| **P1** | **Audio System 完善** — SDL3/XAudio2 后端 | 3 | 3 | 3 | 4 | **13** | ✅ 50% |
| **P1** | **Swappy 集成** — Android 帧率控制 | 2 | 3 | 3 | 3 | **13** | 📋 待开始 |
| **P1** | **Editor Tools (ImGui)** — 从 15% 起步 | 4 | 3 | 2 | 3 | **12** | ⏳ 15% |
| **P1** | **Physics Engine** — JoltPhysics 集成 | 4 | 3 | 3 | 4 | **12** | 📋 待开始 |
| **P1** | **MCP Protocol 收尾** — 最后 15% | 2 | 2 | 2 | 4 | **12** | ✅ 85% |
| **P1** | **Firefly Clamping** — 路径追踪色彩钳制 | 1 | 1 | 2 | 2 | **8→12** | 📋 成本极低 |

### P1.1 — Scene Serialization

**依赖关系**: WebUI Editor 和 ImGui Editor 都需要场景序列化才能真正编辑场景。

| Task | 文件 | 关键产出 |
|------|------|---------|
| 1 | ComponentRegistry.h/.cpp | 类型注册系统 |
| 2 | Component.h | GetComponentTypeName() 虚方法 |
| 3 | Transform.h/.cpp | Transform::Data + 序列化 |
| 4 | GameObject.h/.cpp | GameObject Data + 序列化 |
| 5 | Scene.h/.cpp | 完整 JSONC 场景 I/O |
| 6 | SpriteRenderer.h/.cpp | 组件序列化示例 |
| 7 | 场景文件 .json → .jsonc | 新场景格式 |

### P1.2 — Audio System 完善

- XAudio2/SDL3 后端已定义架构
- 需完成播放、控制功能的完整实现
- 3D 空间音频支持

### P1.3 — Swappy 集成 (Android)

- Google Swappy SDK 集成
- Android 帧率控制，显著提升用户体验
- 官方 SDK，集成成本低

### P1.4 — Firefly Clamping（随手做）

```glsl
color = min(color, vec3(100.0));  // tone mapping 前
// 或
color = clamp(color, 0.0, 20.0);  // 累积时
```

**成本**: 一行代码改动，建议随 NEE 一起提交。

---

## 🟢 Tier 2 — 后续（综合分 8-11）

| 优先级 | 项 | 复杂度 | 影响范围 | 阻碍程度 | 利益相关者 | 总分 | 状态 |
|--------|-----|--------|---------|---------|-----------|------|------|
| **P2** | **C# UI System Phase 1** — UIComponent/Image/Button/Text | 3 | 3 | 2 | 3 | **11** | 📋 设计完成 |
| **P2** | **DX12 Backend 完善** — 从 70% 到 100% | 5 | 4 | 2 | 4 | **11** | ✅ 70% |
| **P2** | **Shaders 完善** — PBR 着色器全集成 | 3 | 3 | 2 | 3 | **11** | ✅ 50% |
| **P2** | **Snappy 集成** — 快速压缩库 | 2 | 3 | 2 | 3 | **10** | 📋 待开始 |
| **P2** | **2D 渲染增强** — Vulkan 加速 SSR / 法线贴图 / SDF 阴影 | 4 | 2 | 2 | 3 | **9** | 📋 设计完成 |
| **P2** | **Crashpad 集成** — 崩溃报告系统 | 3 | 2 | 2 | 3 | **8** | 📋 待开始 |

---

## ⬜ Tier 3 — 远期（综合分 < 8）

| 优先级 | 项 | 复杂度 | 影响范围 | 阻碍程度 | 利益相关者 | 总分 | 状态 |
|--------|-----|--------|---------|---------|-----------|------|------|
| **P3** | **RenderGraph 迁移** — Phase 1-3（12 周工程） | 5 | 3 | 2 | 3 | **9** | 📋 设计完成 |
| **P3** | **C# UI System Phase 2-3** — Canvas/Panel/ScrollView/Flexbox | 4 | 2 | 2 | 2 | **8** | 📋 计划中 |
| **P3** | **Animation System** — 骨骼/蒙皮/动画状态机 | 5 | 2 | 2 | 3 | **7** | 📋 待开始 |
| **P3** | **9 Render Features** — Bloom/AA/SSAO/UI/Volumetric 等 | 4 | 3 | 1 | 2 | **7** | 📋 已规划 |
| **P3** | **Denoiser** — 路径追踪降噪 (SVGF/A-SVGF/BMFR) | 4 | 1 | 1 | 2 | **6** | 📋 待开始 |
| **P3** | **HAP Video** — GPU 加速视频播放 | 3 | 1 | 1 | 1 | **6** | 📋 待开始 |
| **P3** | **Deferred Rendering** — 延迟渲染管线 | 4 | 2 | 1 | 2 | **6** | 📋 已规划 |
| **P3** | **HLSL → SPIR-V** | 3 | 1 | 1 | 1 | **5** | 📋 待开始 |

---

## 依赖关系图

```
Resource Management (P0.3) ───── 无阻塞
       │
       ▼
Scene Serialization (P1.1) ──── 编辑器依赖此项
       │
       ├──► WebUI Editor 增强
       └──► ImGui Editor 增强
       
Pass 实现 (P0.1) ──────────── 渲染管线基座
       │
       ├──► PBR 集成 (P0.2)
       ├──► ShadowPass 完整
       └──► Deferred Rendering (P3) ← 依赖 Pass 架构稳定

MCP Protocol (P1.7) ───────── 独立，可并行
Audio System (P1.2) ───────── 独立，可并行
Swappy (P1.3) ────────────── 独立，可并行
Physics (P1.6) ───────────── 独立，可并行
```

---

## 并行执行建议

以下任务互不依赖，可并行推进：

| 并行组 | 任务 |
|--------|------|
| **组 A** | Pass 实现 + PBR 集成 + Resource 收尾 |
| **组 B** | Scene Serialization + Audio + Swappy |
| **组 C** | NEE + Firefly Clamping（路径追踪） |
| **组 D** | Editor Tools + MCP 收尾 |
| **组 E** | Physics + Snappy + Crashpad |

---

## 全部待办项一览（按优先级排序）

- [ ] **P0** Pass 实现（Opaque/Transparent/Skybox/Shadow/FinalBlit .cpp）
- [ ] **P0** PBR 集成到 OpaquePass
- [ ] **P0** Resource Management 收尾 → 100%
- [ ] **P1** Scene Serialization（ComponentRegistry + JSONC）
- [ ] **P1** NEE（Next Event Estimation）
- [ ] **P1** Audio System 完善（SDL3/XAudio2）
- [ ] **P1** Swappy 集成（Android 帧率控制）
- [ ] **P1** Editor Tools (ImGui) 扩展
- [ ] **P1** Physics Engine (JoltPhysics)
- [ ] **P1** MCP Protocol 收尾 → 100%
- [ ] **P1** Firefly Clamping
- [ ] **P2** C# UI System Phase 1
- [ ] **P2** DX12 Backend 完善
- [ ] **P2** Shaders 完善
- [ ] **P2** Snappy 集成
- [ ] **P2** 2D 渲染增强（Vulkan SSR / 法线贴图 / SDF 阴影）
- [ ] **P2** Crashpad 崩溃报告
- [ ] **P3** RenderGraph 迁移
- [ ] **P3** C# UI System Phase 2-3
- [ ] **P3** Animation System
- [ ] **P3** 9 Render Features（Bloom/AA/SSAO 等）
- [ ] **P3** Denoiser（SVGF）
- [ ] **P3** HAP Video
- [ ] **P3** Deferred Rendering
- [ ] **P3** HLSL → SPIR-V

---

## 更新日志

| 日期 | 变更 |
|------|------|
| 2026-06-04 | 初始创建，基于 Godot 四维框架对所有待办项排序 |
