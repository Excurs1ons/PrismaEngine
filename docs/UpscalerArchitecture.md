# Prisma Engine 超分辨率架构文档

## 概述

Prisma Engine 超分辨率框架是一个可插拔的、模块化的超分辨率技术支持系统，支持多种业界领先的超分辨率技术：

- **FSR 3.1 / FSR Redstone** - AMD FidelityFX Super Resolution（跨平台）
- **DLSS 4.5** - NVIDIA Deep Learning Super Sampling（Windows/Linux）
- **TSR** - Temporal Super Resolution（自实现，跨平台）

## 设计原则

1. **接口统一** - 所有超分技术实现统一的 `IUpscaler` 接口
2. **可插拔架构** - 通过适配器模式支持多种技术
3. **条件编译** - CMake 控制静态库链接，最小化程序体积
4. **运行时可查询** - 提供接口查询已编译的超分模式
5. **运行时可切换** - 在已编译的模式之间无缝切换

---

## 架构图

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Prisma Engine 渲染管线                        │
└─────────────────────────────────────────────────────────────────────┘
                                    │
        ┌───────────────────────────┼───────────────────────────┐
        │                           │                           │
        ▼                           ▼                           ▼
┌───────────────┐           ┌───────────────┐           ┌───────────────┐
│ GeometryPass  │           │ LightingPass  │           │ UpscalerPass  │
│ MotionVectors │           │  Composition  │           │  (Optional)   │
└───────────────┘           └───────────────┘           └───────────────┘
        │                           │                           │
        └───────────────────────────┼───────────────────────────┘
                                    │
                                    ▼
                        ┌─────────────────────────────┐
                        │    UpscalerManager (单例)    │
                        └─────────────────────────────┘
                                    │
        ┌───────────────────────────┼───────────────────────────┐
        │                           │                           │
        ▼                           ▼                           ▼
┌───────────────┐           ┌───────────────┐           ┌───────────────┐
│ UpscalerFSR   │           │ UpscalerDLSS  │           │ UpscalerTSR   │
│  (适配器)     │           │  (适配器)     │           │  (适配器)     │
└───────────────┘           └───────────────┘           └───────────────┘
        │                           │                           │
        ▼                           ▼                           ▼
┌───────────────┐           ┌───────────────┐           ┌───────────────┐
│ FSRResources  │           │ DLSSResources │           │ (自管理资源)  │
└───────────────┘           └───────────────┘           └───────────────┘
        │                           │                           │
        ▼                           ▼                           ▼
┌───────────────┐           ┌───────────────┐           ┌───────────────┐
│ FidelityFX    │           │ Streamline    │           │   (无 SDK)    │
│ SDK 2.1.0     │           │ SDK 2.9.0     │           │               │
└───────────────┘           └───────────────┘           └───────────────┘
```

---

## 核心组件

### 1. IUpscaler 接口

**位置**: `src/engine/graphic/interfaces/IUpscaler.h`

所有超分辨率器必须实现的统一接口。

#### 核心方法

| 类别 | 方法 | 说明 |
|------|------|------|
| **生命周期** | `Initialize()` | 初始化超分辨率器 |
| | `Shutdown()` | 关闭并释放资源 |
| | `IsInitialized()` | 查询初始化状态 |
| **渲染执行** | `Upscale()` | 执行超分辨率处理 |
| **配置管理** | `SetQualityMode()` | 设置质量模式 |
| | `SetRenderResolution()` | 设置渲染分辨率 |
| | `SetDisplayResolution()` | 设置显示分辨率 |
| **查询接口** | `GetInfo()` | 获取超分器信息 |
| | `IsQualityModeSupported()` | 检查质量模式支持 |
| | `GetRecommendedRenderResolution()` | 获取推荐渲染分辨率 |
| **资源管理** | `OnResize()` | 处理分辨率变化 |
| | `ReleaseResources()` | 释放 GPU 资源 |
| **调试功能** | `ResetHistory()` | 重置历史数据 |
| | `GetDebugInfo()` | 获取调试信息 |

#### 枚举定义

```cpp
enum class UpscalerQuality : uint32_t {
    None = 0,
    UltraQuality = 1,     // 1.3x 缩放
    Quality = 2,          // 1.5x 缩放
    Balanced = 3,         // 1.7x 缩放
    Performance = 4,      // 2.0x 缩放
    UltraPerformance = 5  // 3.0x 缩放
};

enum class UpscalerTechnology : uint32_t {
    None = 0,
    FSR = 1,    // AMD FidelityFX Super Resolution
    DLSS = 2,   // NVIDIA Deep Learning Super Sampling
    TSR = 3     // Temporal Super Resolution
};
```

#### 数据结构

```cpp
// 输入描述
struct UpscalerInputDesc {
    ITexture* colorTexture = nullptr;          // 颜色输入（必需）
    ITexture* depthTexture = nullptr;          // 深度输入（必需）
    ITexture* motionVectorTexture = nullptr;   // 运动矢量（必需）
    ITexture* normalTexture = nullptr;         // 法线（可选）
    ITexture* exposureTexture = nullptr;       // 曝光（DLSS 必需）
    float jitterX = 0.0f;                      // 子像素抖动 X
    float jitterY = 0.0f;                      // 子像素抖动 Y
    float deltaTime = 0.0f;                    // 帧时间
    bool resetAccumulation = false;            // 重置累积
    struct {
        PrismaMath::mat4 view;
        PrismaMath::mat4 projection;
        PrismaMath::mat4 viewProjection;
        PrismaMath::mat4 prevViewProjection;
    } camera;
};

// 输出描述
struct UpscalerOutputDesc {
    IRenderTarget* outputTarget = nullptr;     // 输出目标
    uint32_t outputWidth = 0;
    uint32_t outputHeight = 0;
    bool sharpnessEnabled = false;
    float sharpness = 0.5f;
};

// 初始化描述
struct UpscalerInitDesc {
    uint32_t renderWidth = 0;
    uint32_t renderHeight = 0;
    uint32_t displayWidth = 0;
    uint32_t displayHeight = 0;
    UpscalerQuality quality = UpscalerQuality::Quality;
    bool enableHDR = false;
    uint32_t maxFramesInFlight = 2;
};
```

---

### 2. UpscalerManager（单例管理器）

**位置**: `src/engine/graphic/upscaler/UpscalerManager.h/.cpp`

#### 职责

1. 创建和管理所有超分辨率器实例
2. 提供运行时技术查询
3. 管理活动超分辨率器
4. 提供 Helper 函数

#### 核心 API

```cpp
class UpscalerManager {
public:
    // 获取单例
    static UpscalerManager& Instance();

    // 初始化管理器
    void Initialize(IRenderDevice* device);
    void Shutdown();

    // 查询可用技术
    std::vector<UpscalerTechnology> GetAvailableTechnologies() const;
    bool IsTechnologyAvailable(UpscalerTechnology technology) const;

    // 创建/获取超分辨率器
    IUpscaler* CreateUpscaler(UpscalerTechnology technology,
                              const UpscalerInitDesc& desc);
    IUpscaler* GetUpscaler(UpscalerTechnology technology) const;

    // 活动超分辨率器管理
    IUpscaler* GetActiveUpscaler() const;
    void SetActiveUpscaler(IUpscaler* upscaler);

    // 静态辅助方法
    static UpscalerTechnology GetDefaultTechnology();
    UpscalerInfo GetTechnologyInfo(UpscalerTechnology technology) const;
    bool IsInitialized() const { return m_initialized; }
};
```

#### 平台默认配置

| 平台 | 默认技术 | 备选顺序 |
|------|----------|----------|
| Windows | DLSS | DLSS → FSR → TSR |
| Android | FSR | FSR → TSR |
| Linux | DLSS | DLSS → FSR → TSR |

---

### 3. UpscalerHelper 工具类

**位置**: `src/engine/graphic/upscaler/UpscalerManager.cpp`（命名空间）

提供超分辨率系统所需的工具函数。

#### Halton 序列生成

用于生成低差异的子像素抖动序列，提高时序累积质量。

```cpp
namespace UpscalerHelper {
    // 生成 Halton(2,3) 序列
    void GenerateHaltonSequence(int index, float& x, float& y);

    // 获取技术名称
    std::string GetTechnologyName(UpscalerTechnology technology);

    // 获取质量模式名称
    std::string GetQualityName(UpscalerQuality quality);

    // 获取缩放因子
    float GetScaleFactor(UpscalerQuality quality);

    // 计算推荐渲染分辨率
    void CalculateRenderResolution(UpscalerQuality quality,
                                    uint32_t displayWidth,
                                    uint32_t displayHeight,
                                    uint32_t& outWidth,
                                    uint32_t& outHeight);
}
```

---

## 适配器实现

### FSR 适配器

**位置**: `src/engine/graphic/upscaler/adapters/FSR/`

#### 文件结构

```
FSR/
├── UpscalerFSR.h/.cpp      # FSR 适配器实现
└── FSRResources.h/.cpp      # FSR 资源管理
```

#### 特性

- **SDK**: AMD FidelityFX SDK v2.1.0
- **质量模式**: NativeAA, Quality, Balanced, Performance, UltraPerformance
- **必需输入**: 颜色、深度、运动矢量
- **可选输入**: 曝光、法线
- **资源管理**:
  - 双缓冲历史（颜色、深度）
  - 自动曝光计算
  - 锁定掩码（反闪烁）
  - RCAS 输出

---

### DLSS 适配器

**位置**: `src/engine/graphic/upscaler/adapters/DLSS/`

#### 文件结构

```
DLSS/
├── UpscalerDLSS.h/.cpp      # DLSS 适配器实现
└── DLSSResources.h/.cpp      # DLSS 资源管理
```

#### 特性

- **SDK**: NVIDIA Streamline SDK v2.9.0
- **质量模式**: Off, Ultra Quality, Quality, Balanced, Performance, Ultra Performance
- **必需输入**: 颜色、深度、运动矢量、**曝光（必需）**
- **平台限制**: Windows (DX12), Linux (Vulkan)
- **资源管理**:
  - 双缓冲历史（颜色）
  - Streamline 管理的着色器

---

### TSR 适配器

**位置**: `src/engine/graphic/upscaler/adapters/TSR/`

#### 文件结构

```
TSR/
└── UpscalerTSR.h/.cpp       # TSR 适配器实现（自实现）
```

#### 特性

- **SDK**: 无（自实现）
- **质量模式**: Ultra Quality, Quality, Balanced, Performance, UltraPerformance
- **必需输入**: 颜色、深度、运动矢量
- **可选输入**: 法线
- **资源管理**:
  - 双缓冲历史（颜色、深度）
  - 常量缓冲区
  - 自管理着色器

#### TSR 特定参数

```cpp
void SetTemporalStability(float stability);  // 时序稳定性 [0-1]
float GetTemporalStability() const;
```

---

## 渲染管线集成

### UpscalerPass

**位置**: `src/engine/graphic/upscaler/UpscalerPass.h/.cpp`

#### 职责

1. 在渲染管线中执行超分辨率
2. 管理抖动序列（Halton 序列）
3. 连接输入资源和输出目标
4. 支持运行时切换超分技术

#### 执行流程

```
1. 更新阶段 (Update)
   └── 生成下一帧抖动偏移

2. 执行阶段 (Execute)
   ├── 验证输入
   ├── 准备输入描述
   ├── 准备输出描述
   ├── 调用超分辨率器
   └── 更新上一帧投影矩阵
```

#### API

```cpp
class UpscalerPass : public IPass {
public:
    // 设置输出
    void SetRenderTarget(IRenderTarget* renderTarget);

    // 设置输入
    void SetColorInput(ITexture* color);
    void SetDepthInput(ITexture* depth);
    void SetMotionVectors(ITexture* motionVectors);

    // 配置
    bool SetUpscaler(UpscalerTechnology technology);
    void SetQualityMode(UpscalerQuality quality);
    void SetViewport(uint32_t width, uint32_t height);

    // 相机信息
    void UpdateCameraInfo(const PrismaMath::mat4& view,
                          const PrismaMath::mat4& projection,
                          const PrismaMath::mat4& prevViewProjection);

    // 抖动
    void GetJitterOffset(float& x, float& y) const;
};
```

---

### MotionVectorPass

**位置**: `src/engine/graphic/upscaler/MotionVectorPass.h/.cpp`

#### 职责

1. 从深度和相机信息生成屏幕空间运动矢量
2. 支持静态和动态物体
3. 输出用于超分技术的运动矢量纹理

#### 输入

- 当前深度缓冲
- 上一帧深度缓冲
- GBuffer（位置数据）
- 相机信息（当前和上一帧视图投影矩阵）

#### 输出

- 运动矢量纹理（RG16_Float 格式）

---

## CMake 构建系统

### UpscalerOptions.cmake

**位置**: `cmake/UpscalerOptions.cmake`

#### 配置选项

```cmake
# FSR 支持
option(PRISMA_ENABLE_UPSCALER_FSR "Enable AMD FidelityFX Super Resolution" ON)

# DLSS 支持
option(PRISMA_ENABLE_UPSCALER_DLSS "Enable NVIDIA DLSS" ON)

# TSR 支持
option(PRISMA_ENABLE_UPSCALER_TSR "Enable Temporal Super Resolution" ON)

# 运动矢量生成
option(PRISMA_ENABLE_UPSCALER_MOTION_VECTORS "Enable motion vectors generation" ON)

# 调试可视化
option(PRISMA_ENABLE_UPSCALER_DEBUG "Enable upscaler debug visualization" OFF)
```

#### 平台默认配置

| 平台 | FSR | DLSS | TSR |
|------|-----|------|-----|
| Windows | ✅ | ✅ | ✅ |
| Android | ✅ | ❌ | ✅ |
| Linux | ✅ | ✅ | ✅ |

---

### SDK 依赖管理

**位置**: `cmake/FetchThirdPartyDeps.cmake`

#### FidelityFX SDK v2.1.0

```cmake
FetchContent_Declare(
    FidelityFX-SDK
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK.git
    GIT_TAG v2.1.0
    GIT_SHALLOW TRUE
)
```

#### Streamline SDK v2.9.0

```cmake
FetchContent_Declare(
    Streamline
    GIT_REPOSITORY https://github.com/NVIDIA-RTX/Streamline.git
    GIT_TAG v2.9.0
    GIT_SHALLOW TRUE
)
```

---

## 数据流

### 超分辨率执行流程

```
┌─────────────────────────────────────────────────────────────────┐
│                      渲染管线执行                                │
└─────────────────────────────────────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│GeometryPass  │    │LightingPass  │    │Composition   │
│+ MotionVector│    │              │    │   Pass       │
└──────────────┘    └──────────────┘    └──────────────┘
        │                   │                   │
        └───────────────────┼───────────────────┘
                            │
                    ┌───────▼───────┐
                    │ UpscalerPass  │
                    └───────┬───────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│ 颜色输入       │   │ 深度输入       │   │ 运动矢量输入   │
│ Color         │   │ Depth         │   │ MotionVectors │
└───────────────┘   └───────────────┘   └───────────────┘
        │                   │                   │
        └───────────────────┼───────────────────┘
                            ▼
                    ┌───────────────┐
                    │ IUpscaler     │
                    │ (活动实例)     │
                    └───────┬───────┘
                            │
                            ▼
                    ┌───────────────┐
                    │ 超分辨率处理   │
                    │ (FSR/DLSS/TSR)│
                    └───────┬───────┘
                            │
                            ▼
                    ┌───────────────┐
                    │ 输出目标       │
                    │ Output Target │
                    └───────────────┘
```

---

## 内存管理

### 资源管理策略

1. **智能指针**: 所有 GPU 资源使用 `std::unique_ptr` 管理
2. **RAII 模式**: 资源在析构时自动释放
3. **显式释放**: 提供 `ReleaseResources()` 用于立即释放

### 示例

```cpp
// FSR 资源管理
class FSRResources {
private:
    std::unique_ptr<ITexture> m_colorInput;
    std::unique_ptr<ITexture> m_depthInput;
    std::vector<std::unique_ptr<ITexture>> m_historyColor;

    // 析构时自动释放
    ~FSRResources() {
        Release();
    }
};
```

---

## 性能考虑

### 质量模式与性能

| 质量模式 | 缩放因子 | 渲染负载 | 相对性能 |
|----------|----------|----------|----------|
| Ultra Quality | 1.3x | 高 | ~80% |
| Quality | 1.5x | 中高 | ~100% (基准) |
| Balanced | 1.7x | 中 | ~120% |
| Performance | 2.0x | 中低 | ~150% |
| Ultra Performance | 3.0x | 低 | ~200% |

### 抖动序列

- **Halton(2,3)**: 16 帧循环
- **范围**: [-0.5, 0.5] 像素
- **目的**: 时序抗锯齿和超分累积

---

## 调试功能

### 调试信息

```cpp
std::string debugInfo = upscaler->GetDebugInfo();
// 输出示例：
// DLSS 4.5 Upscaler:
//   Initialized: Yes
//   Render Resolution: 1280x720
//   Display Resolution: 1920x1080
//   Quality Mode: Quality
//   Frame Index: 1234
```

### 可视化模式（TODO）

- 运动矢量可视化
- 深度缓冲可视化
- 历史帧差异可视化
- 锐化强度可视化

---

## 线程安全

### 当前状态

- **UpscalerManager**: 单例访问，**非线程安全**
- **IUpscaler**: 接口设计为单线程使用
- **GPU 资源**: 由图形 API 管理线程安全

### 生产建议

```cpp
class UpscalerManager {
private:
    std::mutex m_mutex;  // 添加互斥锁保护

public:
    void SetActiveUpscaler(IUpscaler* upscaler) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_activeUpscaler = upscaler;
    }
};
```

---

## 扩展指南

### 添加新的超分辨率技术

1. **创建适配器类**，继承 `IUpscaler`
2. **实现所有虚方法**
3. **在 UpscalerManager 中注册**
4. **添加 CMake 选项**
5. **更新枚举定义**

### 示例

```cpp
// 1. 创建适配器
class UpscalerXGAMING : public IUpscaler {
public:
    bool Initialize(const UpscalerInitDesc& desc) override;
    bool Upscale(IDeviceContext* context,
                 const UpscalerInputDesc& input,
                 const UpscalerOutputDesc& output) override;
    // ... 其他方法
};

// 2. 添加枚举
enum class UpscalerTechnology : uint32_t {
    // ...
    XGAMING = 4
};

// 3. 在 UpscalerManager 中注册
void UpscalerManager::CreateAvailableUpscalers(IRenderDevice* device) {
#if defined(PRISMA_ENABLE_UPSCALER_XGAMING)
    m_upscalers[UpscalerTechnology::XGAMING] =
        std::make_unique<UpscalerXGAMING>();
#endif
}
```

---

## 参考资料

### SDK 文档

- [AMD FidelityFX SDK v2.1.0](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK)
- [NVIDIA Streamline SDK v2.9.0](https://github.com/NVIDIA-RTX/Streamline)
- [DLSS Developer Resources](https://developer.nvidia.com/rtx/dlss)

### 技术论文

- "Deep Learning Super-Sampling" (NVIDIA, 2020-2025)
- "FidelityFX Super Resolution 3.1" (AMD, 2024)
- "Temporal Super Resolution" (Unreal Engine 5, 2021-2025)

### 相关文档

- [渲染系统文档](RenderingSystem.md)
- [资产序列化](AssetSerialization.md)
- [Vulkan 集成](VulkanIntegration.md)
