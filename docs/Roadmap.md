# 🎮 PrismaEngine 功能路线图

## 📋 项目概述
PrismaEngine（原YAGE）是一个为现代游戏开发设计的跨平台游戏引擎，支持Windows和Android平台，使用DirectX 12和Vulkan作为渲染后端，集成SDL3实现跨平台窗口和输入。

## 🔄 Vulkan 后端迁移计划

> **重要**: [PrismaAndroid](https://github.com/Excurs1ons/PrismaAndroid) 项目包含一个功能完整的 Vulkan 运行时实现（~1300行代码），将逐步迁移到 PrismaEngine 的渲染抽象层。

### 迁移阶段

| 阶段 | 内容 | 状态 |
|------|------|------|
| **Phase 1** | 渲染抽象层设计 | 🔄 进行中 |
| **Phase 2** | VulkanContext 迁移 | ⏳ 计划中 |
| **Phase 3** | RendererVulkan 迁移 | ⏳ 计划中 |
| **Phase 4** | Shader/Texture 迁移 | ⏳ 计划中 |
| **Phase 5** | 集成测试与优化 | ⏳ 计划中 |

### PrismaAndroid 功能清单

| 模块 | 完成度 | 说明 |
|------|--------|------|
| VulkanContext | ✅ 100% | Instance/Device/SwapChain/缓冲区管理 |
| RendererVulkan | ✅ 90% | 完整渲染循环，支持屏幕旋转 |
| ShaderVulkan | ✅ 100% | SPIR-V 着色器加载 |
| TextureAsset | ✅ 100% | 纹理加载、Mipmap、采样器 |
| Scene/GameObject | ✅ 80% | 简化版 ECS 架构 |
| 命令缓冲管理 | ✅ 100% | Flight Frame 同步机制 |
| 描述符系统 | ✅ 100% | Pool/Sets/Layout 管理 |

## 🚀 高级渲染功能规划

### 1. HLSL → SPIR-V 统一着色器编译流程

#### 背景
当前 DirectX 12 使用 HLSL，Vulkan 使用 GLSL，导致着色器代码维护两套。通过使用 `dxc` (DXC Compiler) 将 HLSL 编译为 SPIR-V，可实现跨平台统一开发流程。

#### 技术方案

| 组件 | 说明 |
|------|------|
| **编译器** | Microsoft DXC (DirectX Shader Compiler) |
| **输入格式** | HLSL (Shader Model 6.0+) |
| **输出格式** | SPIR-V (Vulkan 1.1+) |
| **构建工具** | CMake + 自定义脚本 |

#### 实现阶段

| 阶段 | 内容 | 状态 |
|------|------|------|
| **Phase 1** | DXC 工具链集成 (CMake + 构建脚本) | ⏳ 计划中 |
| **Phase 2** | HLSL 着色器编写规范与示例 | ⏳ 计划中 |
| **Phase 3** | 运行时 SPIR-V 加载器完善 | ⏳ 计划中 |
| **Phase 4** | 跨平台着色器热重载 | ⏳ 计划中 |

#### 目录结构
```
assets/shaders/
├── hlsl/              # HLSL 源码 (统一编写)
│   ├── common.hlsl    # 共享头文件
│   ├── vertex.hlsl    # 顶点着色器
│   └── fragment.hlsl  # 像素着色器
├── spirv/             # 编译输出的 SPIR-V (自动生成)
│   ├── vertex.vert.spv
│   └── fragment.frag.spv
└── compiled_dx12/     # DirectX 12 DXIL (可选)
```

#### 关键技术点
- HLSL 到 SPIR-V 的语义映射 (`SV_Position` → `gl_Position`)
- 资源绑定模型转换 (Descriptor Set 自动分配)
- 跨平台着色器反射系统
- CMake 构建时自动编译

---

### 2. Google Swappy 帧率管理集成

#### 背景
[Swappy](https://github.com/google/swappy) 是 Google 开源的 Android 帧率管理库，提供：
- 流畅帧率同步
- 自动 SwapInterval 调整
- 降低功耗和延迟
- 支持 Choreographer 和 Vulkan

#### 功能规划

| 功能 | 说明 | 优先级 |
|------|------|--------|
| **帧率同步** | 自动适配屏幕刷新率 (60/90/120Hz) | 高 |
| **Swap 调度** | 优化 SwapChain 呈现时机 | 高 |
| **性能统计** | FPS/帧时间/掉帧统计 | 中 |
| **功耗优化** | 自动降帧以降低功耗 | 中 |
| **Windows 移植** | 参考 Swappy 实现 Windows 版本 | 低 |

#### 实现阶段

| 阶段 | 内容 | 状态 |
|------|------|------|
| **Phase 1** | Swappy 库集成 (Android) | ⏳ 计划中 |
| **Phase 2** | Vulkan SwapChain 适配 | ⏳ 计划中 |
| **Phase 3** | 性能统计面板 | ⏳ 计划中 |
| **Phase 4** | Windows 后端实现 | ⏳ 计划中 |

#### 架构设计
```
src/engine/graphic/
├── PresentScheduler.h        # 呈现调度器抽象
├── SwappyScheduler.h/cpp     # Android Swappy 实现
├── WindowsScheduler.h/cpp    # Windows D3D12 实现
└── FrameStats.h/cpp          # 帧率统计
```

#### 关键技术点
- ANativeWindowCallback 回调集成
- Vulkan QueuePresentKHR 拦截
- Choreographer 同步
- 自适应刷新率切换

---

### 3. HAP 视频播放系统

#### 背景
[HAP](https://github.com/vidvox/hap) 是高性能 GPU 加速视频编解码格式，特点：
- 基于纹理压缩 (DXT/S3TC/BPTC)
- GPU 直接解码，CPU 开销极低
- 适合实时视频播放和互动媒体
- 广泛用于演出和展示行业

#### 系统架构

```
src/engine/video/
├── VideoPlayer.h/cpp          # 视频播放器核心
├── VideoDecoder.h/cpp         # 解码器接口
├── HAPDecoder.h/cpp           # HAP 格式解码器
├── VideoTexture.h/cpp         # 视频纹理输出
└── VideoComponent.h/cpp       # GameObject 组件

third_party/hap/
├── hap_decode/                # HAP 解码库
└── snappy/                    # Snappy 压缩库
```

#### 功能规划

| 功能 | 说明 | 优先级 |
|------|------|--------|
| **HAP 解码** | 支持 HAP/Q/HAPAlpha/HAPQ | 高 |
| **纹理输出** | 解码结果直接输出到 GPU 纹理 | 高 |
| **播放控制** | Play/Pause/Seek/Loop | 高 |
| **音频同步** | 音视频同步播放 | 中 |
| **多轨道** | 支持多视频叠加 | 低 |
| **网络流** | 支持流式加载 | 低 |

#### 实现阶段

| 阶段 | 内容 | 状态 |
|------|------|------|
| **Phase 1** | HAP 解码库集成 (hap-in-c) | ⏳ 计划中 |
| **Phase 2** | VideoPlayer 核心实现 | ⏳ 计划中 |
| **Phase 3** | GPU 纹理直接上传 | ⏳ 计划中 |
| **Phase 4** | VideoComponent 组件 | ⏳ 计划中 |
| **Phase 5** | 音频同步与播放列表 | ⏳ 计划中 |

#### HAP 格式支持

| 格式 | 压缩比 | 质量 | Alpha |
|------|--------|------|-------|
| **HAP** | DXT4 | 中 | 否 |
| **HAP Alpha** | DXT5 + Alpha | 中 | 是 |
| **HAP Q** | BPTC | 高 | 否 |
| **HAP Q Alpha** | BPTC + Alpha | 高 | 是 |

#### 使用示例
```cpp
// 创建视频播放组件
auto videoPlayer = gameObject->addComponent<VideoComponent>();
videoPlayer->load("assets/videos/intro.mov");
videoPlayer->setLoop(true);
videoPlayer->play();

// 输出到渲染纹理
auto outputTexture = videoPlayer->getOutputTexture();
material->setTexture("VideoTexture", outputTexture);
```

#### 关键技术点
- DXT/S3TC 压缩纹理直接上传
- Snappy 解压缩集成
- 帧精确 seeking
- 音视频同步 (PTS 时间戳)

## 🎨 2D功能实现状态

### ✅ 已实现的2D功能

#### 🏗️ 基础架构
- **📦 资源管理系统** - ✅ 已实现
  - 基础资源接口(IResource)和资产基类(Asset)
  - 资源管理器(ResourceManager)支持多路径搜索
  - 支持JSON和二进制序列化
  - 线程安全的资源加载和卸载

- **🧩 组件系统** - ✅ 已实现
  - 基础组件接口(IComponent)和组件基类(Component)
  - GameObject支持组件添加和获取
  - 组件更新机制已实现
  - 组件生命周期管理（Initialize/Update/Shutdown）

#### 🖼️ 渲染系统
- **🎯 DirectX 12渲染器** - ✅ 已实现
  - 完整的DirectX 12渲染管线设置
  - 命令列表和命令队列
  - 渲染目标和视口设置
  - 基础帧同步机制
  - 动态顶点/索引缓冲区上传
  - 动态常量缓冲区支持
  - 交换链、渲染目标、深度缓冲实现
  - 设备丢失处理机制

- **🌟 着色器系统** - ✅ 已实现
  - HLSL着色器编译支持
  - 顶点和像素着色器加载
  - 着色器资源管理

- **🖼️ 纹理系统** - ✅ 已实现
  - 基础纹理资产类(TextureAsset)
  - BMP图像加载支持
  - 纹理属性和元数据管理

#### 📐 几何体
- **📐 基础2D几何体** - ✅ 已实现
  - 三角形网格生成
  - 四边形网格生成
  - 顶点数据结构定义

### 🚧 未完全实现的2D功能

#### 📷 相机系统
- **📷 Camera2D** - ✅ 已实现
  - 类已完整实现
  - 支持2D投影和视图矩阵计算
  - 支持视口和缩放功能
  - 支持正交投影参数设置

#### 🔄 变换系统
- **🔄 Transform2D** - ✅ 已实现
  - 类已完整实现
  - 支持2D位置、旋转和缩放功能
  - 支持变换矩阵计算
  - 支持脏标记优化
  - 支持矢量操作和标量操作

#### 🎨 渲染管线
- **🎨 2D专用渲染管线** - ⚠️ 部分实现
  - RenderPass2D类已定义，但Execute方法为空
  - 缺少2D图形的专用渲染状态
  - 缺少精灵渲染支持
  - 缺少UI渲染支持
  - 缺少2D光照系统

#### 🎭 材质系统
- **🎭 Material** - ✅ 已实现
  - 基础PBR材质属性实现
  - 支持纹理路径设置
  - 与着色器系统集成
  - 基础颜色、金属度、粗糙度、自发光属性
  - 纹理映射支持（反照率、法线、金属度、粗糙度、自发光）
  - ⚠️ 缺少运行时材质编辑器
  - ⚠️ 缺少材质实例系统

#### 🔲 网格渲染
- **🔲 MeshRenderer** - ✅ 已实现
  - 基础结构已定义
  - DrawMesh方法已实现，支持索引和非索引绘制
  - 支持子网格渲染
  - 支持网格和材质绑定
  - 组件生命周期管理
  - ⚠️ 缺少2D网格的专用渲染逻辑
  - ⚠️ 缺少批渲染优化

#### 🎮 输入系统
- **🎮 IInputBackend** - ⚠️ 部分实现
  - 基础输入管理器实现
  - 支持键盘输入检测
  - 支持鼠标输入检测
  - 支持鼠标位置获取
  - ⚠️ 缺少游戏手柄支持
  - ⚠️ 缺少输入映射系统
  - ⚠️ 缺少输入事件处理

## 🌐 3D功能实现状态

### ✅ 已实现的3D功能

#### 🏗️ 基础架构
- **📦 资源管理系统** - ✅ 已实现(同2D)
- **🧩 组件系统** - ✅ 已实现(同2D)

#### 🖼️ 渲染系统
- **🎯 DirectX 12渲染器** - ✅ 已实现(同2D)
- **🔥 Vulkan渲染器** - ⚠️ 部分实现
  - 基础渲染器类定义
  - 已集成到编辑器初始化流程
  - 实现了实例、表面和设备的创建
  - 支持多线程和Bindless纹理特性

- **🌟 着色器系统** - ✅ 已实现(同2D)
- **🎨 渲染管线** - ✅ 已实现
  - 模块化渲染通道设计（RenderPass）
  - 前向渲染管线完整实现
  - 包含深度预通道、不透明通道、透明通道
  - 天空盒渲染通道实现

#### 📐 几何体
- **📦 基础3D几何体** - ✅ 已实现
  - 立方体网格生成
  - 三角形和四边形网格生成
  - 完整的顶点数据结构(位置、法线、纹理坐标、切线、颜色)

#### 🔲 网格系统
- **🔲 Mesh** - ✅ 已实现
  - 子网格支持
  - 包围盒计算
  - 顶点和索引缓冲区结构
  - GPU资源句柄定义

### 🚧 未完全实现的3D功能

#### 🔄 变换系统
- **🔄 Transform** - ✅ 已实现
  - 基于Transform类实现（实际是3D变换）
  - 支持欧拉角和四元数旋转
  - 支持3D位置、旋转和缩放属性
  - 支持矩阵变换计算，使用缓存优化
  - 包含调试日志功能
  - ⚠️ 缺少专门的Transform3D类命名（目前所有变换都通过Transform类处理）

#### 📷 相机系统
- **📷 3D相机** - ✅ 已实现
  - 透视投影相机完整实现
  - 支持LookAt功能
  - 支持本地和世界坐标系移动
  - 实现了前、上、右向量的计算
  - 支持清除颜色设置
  - 使用脏标记优化矩阵更新
  - ⚠️ 缺少正交投影相机实现

#### 🎨 渲染管线
- **🎨 3D专用渲染管线** - ✅ 已实现
  - 深度缓冲和模板缓冲
  - 背面剔除
  - 3D光照系统基础
  - ⚠️ 缺少阴影渲染
  - ⚠️ 渲染图（RenderGraph）目前为空实现
  - ⚠️ 延迟渲染管线存在但可能不完整

#### 🎭 材质系统
- **🎭 3D材质** - ✅ 已实现(同Material系统)
  - PBR材质属性实现
  - 纹理绑定和采样器设置
  - 材质属性与着色器的绑定
  - ⚠️ 缺少材质预览功能
  - ⚠️ 缺少材质库管理
  - ⚠️ 缺少着色器变体系统

#### 🔲 网格渲染
- **🔲 3D网格渲染** - ✅ 已实现
  - 3D网格的专用渲染逻辑
  - MeshRenderer组件实现
  - 支持网格和材质绑定
  - ⚠️ 缺少实例化渲染
  - ⚠️ 缺少LOD系统
  - ⚠️ 缺少批渲染优化
  - ⚠️ 缺少遮挡剔除

#### 🎬 动画系统
- **🎬 骨骼动画** - ❌ 未实现
  - 缺少骨骼和蒙皮系统
  - 缺少动画播放和混合
  - 缺少动画状态机
  - 缺少动画骨骼支持

#### ⚡ 物理系统
- **⚡ 碰撞检测** - ❌ 未实现
  - 缺少碰撞体和碰撞检测
  - 缺少物理模拟
  - 缺少物理材质

## 🌍 跨平台功能

### ✅ 已实现
- **🪟 Windows平台支持** - ⚠️ 部分实现
  - Windows应用程序框架
  - Win32窗口创建和消息处理
  - DirectX 12渲染后端

### 🚧 未完全实现
- **🤖 Android平台支持** - ⚠️ 部分实现
  - Android应用程序类定义
  - 缺少完整的Android实现
  - 缺少Android特定的渲染后端

- **🎮 SDL3集成** - ⚠️ 部分实现
  - SDL3应用程序类定义
  - 缺少完整的SDL3实现
  - 缺少SDL3渲染后端

## 🔊 音频系统

### ✅ 已实现
- **🔊 音频系统架构** - ✅ 已实现
  - 音频系统接口定义
  - 多后端支持架构
  - 支持SDL3和XAudio2后端

### 🚧 未完全实现
- **🎵 SDL3音频后端** - ⚠️ 部分实现
  - 基础SDL3音频初始化
  - 音频播放功能部分实现
  - 音频控制功能未完全实现

- **🎶 XAudio2音频后端** - ⚠️ 部分实现
  - 类定义已存在
  - 部分实现已完成
  - 音频播放功能未完全实现

## ⚡ 物理系统

### 🚧 未完全实现
- **⚛️ JoltPhysics集成** - ❌ 未实现
  - 缺少JoltPhysics库集成
  - 缺少物理世界的创建和管理
  - 缺少碰撞体和刚体组件
  - 缺少物理材质系统
  - 缺少约束和关节系统

## 🚀 高级渲染系统

### 🚧 未完全实现
- **🌟 RTXGI全局光照** - ❌ 未实现
  - 缺少NVIDIA RTXGI SDK集成
  - 缺少DDGI (Dynamic Diffuse Global Illumination) 系统
  - 缺少实时全局光照计算
  - 缺少光照探头和反射捕获
  - 缺少基于硬件的光线追踪支持

## 🛠️ 工具和编辑器

### ✅ 已实现
- **🛠️ 编辑器框架** - ⚠️ 部分实现
  - 编辑器应用程序类定义
  - SDL3窗口创建和初始化
  - Vulkan渲染器集成
  - ImGui界面框架集成
  - 资源管理器集成

### 🚧 未完全实现
- **🛠️ 编辑器功能** - ❌ 未实现
  - 缺少场景编辑器
  - 缺少资源管理器
  - 缺少属性编辑器

- **🐛 崩溃报告系统** - ❌ 未实现
  - 缺少Crashpad集成
  - 缺少崩溃捕获和报告机制
  - 缺少崩溃数据上传功能
  - 缺少符号化工具集成

## 🎯 优先级建议

### 🔴 高优先级
1. **🎨 完善2D基础功能**
   - 实现Transform2D和Camera2D
   - 完善2D渲染管线
   - 实现基础2D材质系统

2. **🌐 完善3D基础功能**
   - 实现Transform3D和Camera3D
   - 完善3D渲染管线
   - 实现基础3D材质系统

3. **🎮 完善输入系统**
   - 实现输入后端
   - 添加输入事件处理

### 🟡 中优先级
1. **🔊 完善音频系统**
2. **🌍 完善跨平台支持**
3. **🛠️ 实现基础编辑器功能**
4. **⚡ 集成JoltPhysics物理引擎**
5. **🐛 集成Crashpad崩溃报告系统**

### 🟢 低优先级
1. **✨ 高级渲染特性**
   - **🚀 RTXGI (Real-Time Global Illumination) 集成**
     - 集成NVIDIA RTXGI SDK
     - 实现基于Voxel Cone Tracing的实时全局光照
     - 支持动态场景下的间接光照反射
     - 添加DDGI (Dynamic Diffuse Global Illumination) 支持
     - 优化移动端和低配设备的性能降级方案
2. **🎬 动画系统**

## 📊 功能完成度统计

| 功能模块 | 完成度 | 状态 |
|---------|--------|------|
| 基础架构 (ECS/Scene/Resource) | 70% | 🟡 进行中 |
| 2D渲染 (Transform2D/Camera2D) | 50% | 🟡 进行中 |
| 3D渲染 (Transform3D/Camera3D) | 45% | 🟡 进行中 |
| DirectX 12 后端 | 65% | 🟡 进行中 |
| **Vulkan 后端 (PrismaAndroid)** | **80%** | 🟢 **已完成** |
| 音频系统 | 15% | 🔴 未开始 |
| 跨平台支持 (Android/SDL3) | 25% | 🔴 未开始 |
| 编辑器工具 | 10% | 🔴 未开始 |
| 物理系统 | 5% | 🔴 未开始 |
| RenderGraph 架构 | 10% | 🔴 规划中 |
| **HLSL → SPIR-V 编译流程** | **0%** | 🔴 **规划中** |
| **Google Swappy 集成** | **0%** | 🔴 **规划中** |
| **HAP 视频播放系统** | **0%** | 🔴 **规划中** |
| 高级渲染 (RTXGI) | 0% | 🔴 未开始 |
| 动画系统 | 0% | 🔴 未开始 |

**总体完成度: ~30-35%**

> **重要更新**: PrismaAndroid 项目包含完整的 Vulkan 渲染器实现（~1300 行），功能包括 SwapChain、图形管线、描述符、Uniform Buffer、屏幕旋转支持等。

> **高级功能规划**: HLSL→SPIR-V 统一着色器流程、Google Swappy 帧率管理、HAP 视频播放系统已列入规划，详见 [高级渲染功能规划](#-高级渲染功能规划) 章节。

## 📝 总结

PrismaEngine（原YAGE）仍处于早期开发阶段，但已有扎实的基础框架和一个**功能完整的 Android Vulkan 运行时**：

### ✅ 已完成的基础框架 (PrismaEngine)
- **ECS组件系统**：GameObject/Component架构完整，支持生命周期管理
- **场景管理**：Scene序列化、SceneManager、SceneNode层次结构
- **相机系统**：Camera2D/Camera3D完整实现，包含控制器
- **变换系统**：Transform/Transform2D完整实现，支持矩阵缓存优化
- **DirectX 12 后端**：命令列表、队列、交换链、RT/DS缓冲
- **资源管理**：ResourceManagerNew，支持异步加载和热重载
- **ScriptableRenderPipeline**：前向渲染管线框架（深度预通道、不透明/透明通道、天空盒）

### ✅ **已完成: PrismaAndroid Vulkan 运行时**
- **VulkanContext** (~250 行)：
  - Instance/Device/Surface 创建
  - SwapChain 管理
  - 缓冲区创建和管理 (createBuffer)
  - 图像布局转换 (transitionImageLayout)
  - Mipmap 生成 (generateMipmaps)
  - 一次性命令执行辅助函数

- **RendererVulkan** (~1300 行)：
  - 完整的 Vulkan 初始化流程
  - 图形管线创建 (Pipeline/RenderPass/Framebuffers)
  - 描述符池和描述符集管理
  - Uniform Buffer 每帧更新
  - 顶点/索引缓冲区管理
  - 纹理加载和采样 (TextureAsset)
  - **屏幕旋转支持** (SwapChain 重建机制)
  - 飞行帧 (Flight Frame) 同步机制
  - 完整的渲染循环

- **配套组件**：
  - Scene/GameObject/Component 架构
  - MeshRenderer 组件
  - ShaderVulkan (SPIR-V 着色器加载)
  - TextureAsset (纹理资产管理)
  - Vertex 结构 (position/color/uv)

### ⚠️ 部分实现的功能
- **输入系统**：基础InputManager，缺少游戏手柄和输入映射
- **音频系统**：架构存在（XAudio2/SDL3），但实现为空
- **材质系统**：PBR材质属性，但缺少运行时编辑器和实例系统

### ❌ 未实现的关键功能
- **物理系统**：PhysicsSystem仅为空壳
- **动画系统**：完全未实现
- **阴影渲染**：未实现
- **后处理效果**：未实现
- **粒子系统**：未实现
- **RenderGraph**：仅有头文件设计（RenderGraphCore.h），无实现
- **脚本系统**：Mono运行时头文件为空
- **编辑器**：Editor框架存在，但无实际UI功能

### 🔄 架构迁移状态
- **RenderGraph 迁移**：处于规划阶段
  - Phase 1: 基础架构实现 - 未开始
  - Phase 2: 集成现有系统 - 未开始
  - Phase 3: 优化和新功能 - 未开始

### 面临的实际挑战
1. **架构设计与实现差距**：RenderGraph设计完善但实现为空
2. **渲染管线功能不完整**：缺少阴影、后处理等关键特性
3. **跨平台验证不足**：Android平台缺少实际测试
4. **调试工具缺失**：无可视化调试和性能分析工具

### 建议
1. **优先 Vulkan 迁移**：将 PrismaAndroid 的成熟实现迁移到引擎，快速获得可用的 Vulkan 后端
2. **聚焦核心渲染**：优先完善前向渲染管线，实现阴影和后处理
3. **渐进式RenderGraph**：可考虑暂缓迁移，先完善现有管线
4. **建立测试体系**：尽早建立渲染测试和验证流程
5. **实用主义**：考虑使用现有中间件（如JoltPhysics、Dear ImGui）减少开发负担

### 🚀 Vulkan 迁移策略

#### 迁移原则
- **保持兼容性**：PrismaAndroid 继续作为独立项目运行
- **渐进式迁移**：逐模块迁移，保持每个阶段可编译运行
- **抽象优先**：先完善渲染抽象层，再适配具体后端

#### 架构对齐

| PrismaAndroid | PrismaEngine | 迁移策略 |
|---------------|--------------|----------|
| `VulkanContext` | `RenderBackend` + `VulkanDevice` | 抽取接口，保留实现 |
| `RendererVulkan` | `VulkanRenderer` | 重构为适配器模式 |
| `ShaderVulkan` | `Shader` + `VulkanShader` | 统一着色器接口 |
| `TextureAsset` | `Texture` + `VulkanTexture` | 统一资源接口 |
| `Scene/GameObject` | 已有 ECS | 保持引擎架构，参考 Android 实现 |

#### 迁移后的目录结构
```
src/engine/graphic/
├── RenderBackend.h           # 渲染后端抽象接口
├── VulkanBackend.h/cpp       # Vulkan 后端实现（迁移自 PrismaAndroid）
├── VulkanContext.h/cpp       # Vulkan 上下文（迁移自 PrismaAndroid）
├── VulkanCommandList.h/cpp   # Vulkan 命令列表封装
├── VulkanShader.h/cpp        # Vulkan 着色器（迁移）
├── VulkanTexture.h/cpp       # Vulkan 纹理（迁移）
└── ...
```

### 🔮 高级渲染功能规划总结

#### 优先级排序

| 功能 | 优先级 | 依赖 |
|------|--------|------|
| **HLSL → SPIR-V** | 🔴 高 | Vulkan 迁移完成 |
| **Google Swappy** | 🟡 中 | Vulkan 迁移完成 |
| **HAP 视频播放** | 🟢 低 | 纹理系统完善 |

#### 技术栈概览

```
┌─────────────────────────────────────────────────────────────┐
│                    高级渲染功能栈                            │
├─────────────────────────────────────────────────────────────┤
│  HLSL ──DXC──► SPIR-V ──► Vulkan/DX12 统一着色器           │
├─────────────────────────────────────────────────────────────┤
│  Swappy ──► 帧率同步 ──► 功耗优化 ──► 性能统计              │
├─────────────────────────────────────────────────────────────┤
│  HAP ──Snappy──► GPU 解码 ──► VideoTexture ──► 材质播放     │
└─────────────────────────────────────────────────────────────┘
```

游戏引擎开发是一项复杂的系统工程，需要持续的投入和耐心。当前项目拥有良好的架构基础，但需要大量实现工作才能达到可用状态。**PrismaAndroid 的 Vulkan 实现为项目提供了一个功能完整的起点，将大大加速跨平台渲染能力的实现。通过 HLSL→SPIR-V 统一着色器流程、Google Swappy 帧率管理优化、以及 HAP 视频播放能力，引擎将具备现代化的高级渲染特性。**

---

*最后更新时间: 2025年12月25日*