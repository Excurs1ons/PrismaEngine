# Prisma Engine 超分辨率 API 文档

## 目录

1. [快速开始](#快速开始)
2. [初始化](#初始化)
3. [运行时 API](#运行时-api)
4. [集成指南](#集成指南)
5. [CMake 配置](#cmake-配置)
6. [平台特定说明](#平台特定说明)
7. [故障排除](#故障排除)
8. [代码示例](#代码示例)

---

## 快速开始

### 最小集成示例

```cpp
#include "graphic/upscaler/UpscalerManager.h"
#include "graphic/upscaler/UpscalerPass.h"

using namespace PrismaEngine::Graphic;

// 1. 初始化管理器
auto& manager = UpscalerManager::Instance();
manager.Initialize(device);

// 2. 创建 UpscalerPass
auto upscalerPass = std::make_unique<UpscalerPass>();
upscalerPass->SetViewport(1920, 1080);

// 3. 集成到渲染管线
pipeline->SetUpscalerPass(upscalerPass.get());

// 4. 在渲染循环中
upscalerPass->SetColorInput(gbuffer->GetAlbedo());
upscalerPass->SetDepthInput(depthBuffer);
upscalerPass->SetMotionVectors(motionVectorTexture);
upscalerPass->Update(deltaTime);
upscalerPass->Execute(context);
```

---

## 初始化

### 1. 管理器初始化

```cpp
#include "graphic/upscaler/UpscalerManager.h"

// 获取单例
auto& manager = UpscalerManager::Instance();

// 初始化（必须在引擎初始化时调用）
manager.Initialize(renderDevice);

// 查询可用技术
auto available = manager.GetAvailableTechnologies();
for (auto tech : available) {
    auto info = manager.GetTechnologyInfo(tech);
    std::cout << "Available: " << info.name << " v" << info.version << std::endl;
}
```

### 2. 创建并配置超分辨率器

```cpp
// 使用默认技术
auto defaultTech = UpscalerManager::GetDefaultTechnology();
IUpscaler* upscaler = manager.GetUpscaler(defaultTech);

// 初始化描述
UpscalerInitDesc desc;
desc.renderWidth = 1280;
desc.renderHeight = 720;
desc.displayWidth = 1920;
desc.displayHeight = 1080;
desc.quality = UpscalerQuality::Quality;
desc.enableHDR = false;
desc.maxFramesInFlight = 2;

// 初始化超分辨率器
if (!upscaler->Initialize(desc)) {
    std::cerr << "Failed to initialize upscaler" << std::endl;
    return;
}
```

---

## 运行时 API

### 技术查询

```cpp
// 检查特定技术是否可用
if (manager.IsTechnologyAvailable(UpscalerTechnology::DLSS)) {
    std::cout << "DLSS is available" << std::endl;
}

// 获取技术信息
auto dlssInfo = manager.GetTechnologyInfo(UpscalerTechnology::DLSS);
std::cout << "Name: " << dlssInfo.name << std::endl;
std::cout << "Version: " << dlssInfo.version << std::endl;
std::cout << "Requires Exposure: " << dlssInfo.requiresExposure << std::endl;
std::cout << "Requires Motion Vectors: " << dlssInfo.requiresMotionVectors << std::endl;

// 获取支持的质量模式
for (auto quality : dlssInfo.supportedQualities) {
    std::cout << "Quality: " << UpscalerHelper::GetQualityName(quality) << std::endl;
}
```

### 质量模式设置

```cpp
IUpscaler* upscaler = manager.GetActiveUpscaler();

// 设置质量模式
if (upscaler->IsQualityModeSupported(UpscalerQuality::Performance)) {
    upscaler->SetQualityMode(UpscalerQuality::Performance);
}

// 获取当前质量模式
auto currentQuality = upscaler->GetQualityMode();
std::cout << "Current Quality: " << UpscalerHelper::GetQualityName(currentQuality) << std::endl;
```

### 分辨率管理

```cpp
// 获取推荐渲染分辨率
uint32_t renderWidth, renderHeight;
upscaler->GetRecommendedRenderResolution(
    UpscalerQuality::Balanced,  // 质量模式
    1920,                       // 显示宽度
    1080,                       // 显示高度
    renderWidth,
    renderHeight
);

std::cout << "Recommended Render Resolution: " << renderWidth << "x" << renderHeight << std::endl;

// 动态更改渲染分辨率
upscaler->SetRenderResolution(renderWidth, renderHeight);

// 动态更改显示分辨率
upscaler->SetDisplayResolution(2560, 1440);
```

### 运行时技术切换

```cpp
// 安全切换技术
auto SwitchToTechnology = [](UpscalerTechnology newTech) -> bool {
    auto& manager = UpscalerManager::Instance();

    // 检查是否可用
    if (!manager.IsTechnologyAvailable(newTech)) {
        std::cerr << "Technology not available" << std::endl;
        return false;
    }

    // 获取新技术实例
    IUpscaler* newUpscaler = manager.GetUpscaler(newTech);
    if (!newUpscaler) {
        std::cerr << "Failed to get upscaler instance" << std::endl;
        return false;
    }

    // 如果需要，重新初始化
    if (!newUpscaler->IsInitialized()) {
        UpscalerInitDesc desc = /* 当前配置 */;
        if (!newUpscaler->Initialize(desc)) {
            std::cerr << "Failed to initialize new upscaler" << std::endl;
            return false;
        }
    }

    // 设置为活动超分辨率器
    manager.SetActiveUpscaler(newUpscaler);

    std::cout << "Switched to " << UpscalerHelper::GetTechnologyName(newTech) << std::endl;
    return true;
};

// 使用示例：从 FSR 切换到 DLSS
SwitchToTechnology(UpscalerTechnology::DLSS);
```

### 重置历史

```cpp
// 场景切换时重置历史
upscaler->ResetHistory();

// 或通过 UpscalerPass
upscalerPass->ResetHistory();
```

---

## 集成指南

### 集成到延迟渲染管线

```cpp
#include "graphic/pipelines/deferred/DeferredPipeline.h"
#include "graphic/upscaler/UpscalerPass.h"

class MyApplication {
private:
    std::unique_ptr<DeferredPipeline> m_pipeline;
    std::unique_ptr<UpscalerPass> m_upscalerPass;
    std::unique_ptr<MotionVectorPass> m_motionVectorPass;

public:
    void Initialize() {
        // 创建管线
        m_pipeline = std::make_unique<DeferredPipeline>();
        m_pipeline->Initialize(device);

        // 创建运动矢量 Pass
        m_motionVectorPass = std::make_unique<MotionVectorPass>();
        m_motionVectorPass->Initialize(device);
        m_pipeline->AddPass(m_motionVectorPass.get());

        // 创建超分辨率 Pass
        m_upscalerPass = std::make_unique<UpscalerPass>();
        m_upscalerPass->SetViewport(1920, 1080);
        m_pipeline->SetUpscalerPass(m_upscalerPass.get());

        // 连接输入
        m_upscalerPass->SetMotionVectors(m_motionVectorPass->GetOutput());
    }

    void RenderFrame() {
        // 更新相机抖动
        float jitterX, jitterY;
        m_upscalerPass->GetJitterOffset(jitterX, jitterY);
        ApplyJitterToProjection(jitterX, jitterY);

        // 更新相机信息
        m_upscalerPass->UpdateCameraInfo(
            camera->GetViewMatrix(),
            camera->GetProjectionMatrix(),
            camera->GetPrevViewProjectionMatrix()
        );

        // 执行管线
        m_pipeline->Execute(context);
    }
};
```

### 生成运动矢量

```cpp
// 方法 1: 使用 MotionVectorPass
m_motionVectorPass->SetCurrentDepth(depthBuffer);
m_motionVectorPass->SetPreviousDepth(prevDepthBuffer);
m_motionVectorPass->SetGBuffer(gbuffer);
m_motionVectorPass->UpdateCameraInfo(view, proj, prevViewProj);
m_motionVectorPass->Execute(context);
auto motionVectors = m_motionVectorPass->GetOutput();

// 方法 2: 在着色器中手动生成
// HLSL 示例
float2 CalculateMotionVector(float3 worldPos, float2 prevClipPos, float2 currClipPos) {
    float2 prevScreenPos = prevClipPos.xy / prevClipPos.w;
    float2 currScreenPos = currClipPos.xy / currClipPos.w;

    float2 velocity = prevScreenPos - currScreenPos;

    // 转换到 [0, 1] 范围
    return velocity * 0.5 + 0.5;
}
```

### 应用抖动到投影矩阵

```cpp
#include "graphic/upscaler/UpscalerManager.h"

void ApplyJitterToProjection(PrismaMath::mat4& projection,
                             IRenderTarget* renderTarget) {
    auto& manager = UpscalerManager::Instance();
    auto* upscalerPass = /* 获取 UpscalerPass */;

    float jitterX, jitterY;
    upscalerPass->GetJitterOffset(jitterX, jitterY);

    // 将抖动转换到 NDC 空间
    float jitterXNDC = jitterX / renderTarget->GetWidth();
    float jitterYNDC = jitterY / renderTarget->GetHeight();

    // 应用到投影矩阵
    projection[2][0] += jitterXNDC;
    projection[2][1] += jitterYNDC;
}
```

### 处理窗口大小变化

```cpp
void OnWindowResize(uint32_t newWidth, uint32_t newHeight) {
    auto& manager = UpscalerManager::Instance();
    IUpscaler* upscaler = manager.GetActiveUpscaler();

    if (upscaler) {
        // 调用 OnResize
        upscaler->OnResize(newWidth, newHeight);

        // 或手动设置
        upscaler->SetDisplayResolution(newWidth, newHeight);

        // 计算新的渲染分辨率
        uint32_t renderWidth, renderHeight;
        upscaler->GetRecommendedRenderResolution(
            upscaler->GetQualityMode(),
            newWidth,
            newHeight,
            renderWidth,
            renderHeight
        );
        upscaler->SetRenderResolution(renderWidth, renderHeight);
    }

    // 更新 UpscalerPass
    if (m_upscalerPass) {
        m_upscalerPass->SetViewport(newWidth, newHeight);
    }
}
```

---

## CMake 配置

### 启用/禁用超分辨率技术

```cmake
# 在 CMakeLists.txt 或命令行中

# 启用 FSR
set(PRISMA_ENABLE_UPSCALER_FSR ON)

# 禁用 DLSS（例如在 Android 上）
set(PRISMA_ENABLE_UPSCALER_DLSS OFF)

# 启用 TSR
set(PRISMA_ENABLE_UPSCALER_TSR ON)
```

### 命令行配置

```bash
# Windows: 只启用 FSR 和 TSR
cmake -DPRISMA_ENABLE_UPSCALER_FSR=ON \
      -DPRISMA_ENABLE_UPSCALER_DLSS=OFF \
      -DPRISMA_ENABLE_UPSCALER_TSR=ON \
      --preset windows-x64-debug

# Android: 启用 FSR 和 TSR（DLSS 不支持）
cmake -DPRISMA_ENABLE_UPSCALER_FSR=ON \
      -DPRISMA_ENABLE_UPSCALER_TSR=ON \
      --preset android-arm64-debug

# Linux: 启用所有技术
cmake -DPRISMA_ENABLE_UPSCALER_FSR=ON \
      -DPRISMA_ENABLE_UPSCALER_DLSS=ON \
      -DPRISMA_ENABLE_UPSCALER_TSR=ON \
      --preset linux-x64-debug
```

### 检查编译时配置

```cpp
// 运行时检查
#if defined(PRISMA_ENABLE_UPSCALER_FSR)
    // FSR 代码
#endif

#if defined(PRISMA_ENABLE_UPSCALER_DLSS)
    // DLSS 代码
#endif

#if defined(PRISMA_ENABLE_UPSCALER_TSR)
    // TSR 代码
#endif

#if defined(PRISMA_ENABLE_UPSCALER_MOTION_VECTORS)
    // 运动矢量代码
#endif
```

---

## 平台特定说明

### Windows

#### DirectX 12 后端

```cpp
// DLSS 在 DX12 上的最佳配置
UpscalerInitDesc desc;
desc.renderWidth = 1280;
desc.renderHeight = 720;
desc.displayWidth = 1920;
desc.displayHeight = 1080;
desc.quality = UpscalerQuality::Quality;
desc.enableHDR = false;  // DLSS HDR 支持需要额外配置
desc.maxFramesInFlight = 3;  // DLSS 推荐至少 3 帧
```

#### 驱动要求

- **DLSS**: NVIDIA GTX 1060 6GB 或更高，驱动 511.23 或更新
- **FSR**: 所有支持 DirectX 12 的 GPU
- **TSR**: 所有支持 DirectX 12 的 GPU

---

### Linux

#### Vulkan 后端

```cpp
// DLSS 在 Vulkan 上的配置
// 注意：DLSS on Linux 需要 Vulkan 1.3+
VkPhysicalDeviceVulkan13Features vulkan13Features = {};
vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
vulkan13Features.computeDerivedDerivativeRates = VK_TRUE;  // DLSS 需要
```

#### 依赖要求

```bash
# 系统包
sudo apt install libvulkan1 vulkan-tools

# 验证 Vulkan 支持
vulkaninfo | grep "deviceName"
```

---

### Android

#### 配置限制

```cmake
# Android 不支持 DLSS
set(PRISMA_ENABLE_UPSCALER_DLSS OFF CACHE BOOL "" FORCE)

# 推荐配置
set(PRISMA_ENABLE_UPSCALER_FSR ON)
set(PRISMA_ENABLE_UPSCALER_TSR ON)
```

#### Gradle 配置

```gradle
// app/build.gradle.kts
android {
    defaultConfig {
        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DPRISMA_ENABLE_UPSCALER_FSR=ON",
                    "-DPRISMA_ENABLE_UPSCALER_TSR=ON"
                )
            }
        }
    }
}
```

#### GPU 兼容性

- **Adreno 600+**: FSR 3.1 完全支持
- **Mali-G700+**: FSR 3.1 完全支持
- **所有 Vulkan GPU**: TSR 支持

---

## 故障排除

### 常见问题

#### 1. DLSS 初始化失败

**问题**: `slDLSSCreate` 返回错误

**解决方案**:
```cpp
// 检查 NVIDIA 驱动版本
// Windows: 检查控制面板中的驱动版本
// 要求: 511.23 或更新

// 检查 GPU 支持
if (!HasNVIDIAGPU()) {
    std::cerr << "DLSS requires NVIDIA GPU" << std::endl;
    return false;
}

// 检查 Vulkan/DX12 支持
// DLSS 需要 DX12 或 Vulkan 1.3+
```

#### 2. FSR 输出有伪影

**问题**: 超分辨率输出有闪烁或重影

**解决方案**:
```cpp
// 确保运动矢量正确
if (!input.motionVectorTexture) {
    std::cerr << "Motion vectors required for FSR" << std::endl;
    return false;
}

// 确保深度缓冲正确
if (!input.depthTexture) {
    std::cerr << "Depth buffer required for FSR" << std::endl;
    return false;
}

// 尝试降低时序稳定性
// FSR 不直接支持，但可以通过重置历史解决
if (sceneChanged) {
    upscaler->ResetHistory();
}
```

#### 3. TSR 性能问题

**问题**: TSR 帧率下降

**解决方案**:
```cpp
// 降低质量模式
upscaler->SetQualityMode(UpscalerQuality::Performance);

// 调整时序稳定性
auto* tsrUpscaler = static_cast<UpscalerTSR*>(upscaler);
tsrUpscaler->SetTemporalStability(0.9f);  // 降低稳定性以提高响应速度
```

#### 4. 运行时切换失败

**问题**: `SetUpscaler` 返回 false

**解决方案**:
```cpp
// 检查技术是否可用
if (!manager.IsTechnologyAvailable(newTech)) {
    std::cerr << "Technology not compiled in" << std::endl;
    return false;
}

// 检查初始化状态
IUpscaler* upscaler = manager.GetUpscaler(newTech);
if (!upscaler->IsInitialized()) {
    UpscalerInitDesc desc = /* 当前配置 */;
    if (!upscaler->Initialize(desc)) {
        std::cerr << "Failed to initialize" << std::endl;
        return false;
    }
}

// 检查输入要求
auto info = upscaler->GetInfo();
if (info.requiresExposure && !exposureTexture) {
    std::cerr << "Exposure texture required" << std::endl;
    return false;
}
```

#### 5. 分辨率计算错误

**问题**: 渲染分辨率为 0 或奇数

**解决方案**:
```cpp
// 使用推荐的计算方法
uint32_t renderWidth, renderHeight;
upscaler->GetRecommendedRenderResolution(
    quality,
    displayWidth,
    displayHeight,
    renderWidth,
    renderHeight
);

// 验证
if (renderWidth == 0 || renderHeight == 0) {
    std::cerr << "Invalid render resolution" << std::endl;
    return false;
}

if (renderWidth % 2 != 0 || renderHeight % 2 != 0) {
    std::cerr << "Render resolution must be even" << std::endl;
    return false;
}
```

---

## 代码示例

### 完整的渲染循环

```cpp
class Renderer {
private:
    std::unique_ptr<DeferredPipeline> m_pipeline;
    std::unique_ptr<UpscalerPass> m_upscalerPass;
    std::unique_ptr<MotionVectorPass> m_motionVectorPass;

    PrismaMath::mat4 m_prevViewProj;

public:
    void Initialize() {
        auto& manager = UpscalerManager::Instance();
        manager.Initialize(m_device);

        // 创建 Pass
        m_motionVectorPass = std::make_unique<MotionVectorPass>();
        m_upscalerPass = std::make_unique<UpscalerPass>();

        // 配置
        m_upscalerPass->SetViewport(1920, 1080);
        m_upscalerPass->SetQualityMode(UpscalerQuality::Quality);

        // 集成到管线
        m_pipeline->AddPass(m_motionVectorPass.get());
        m_pipeline->SetUpscalerPass(m_upscalerPass.get());

        m_prevViewProj = PrismaMath::mat4(1.0f);
    }

    void RenderFrame(float deltaTime) {
        // 1. 更新相机
        auto view = m_camera->GetViewMatrix();
        auto proj = m_camera->GetProjectionMatrix();
        auto viewProj = proj * view;

        // 2. 更新抖动
        m_upscalerPass->Update(deltaTime);
        float jitterX, jitterY;
        m_upscalerPass->GetJitterOffset(jitterX, jitterY);

        // 3. 应用抖动到投影
        auto jitteredProj = ApplyJitter(proj, jitterX, jitterY, 1920, 1080);

        // 4. 更新 Pass 相机信息
        m_upscalerPass->UpdateCameraInfo(view, jitteredProj, m_prevViewProj);
        m_motionVectorPass->UpdateCameraInfo(view, jitteredProj, m_prevViewProj);

        // 5. 设置输入
        m_upscalerPass->SetColorInput(m_gbuffer->GetAlbedo());
        m_upscalerPass->SetDepthInput(m_depthBuffer);
        m_upscalerPass->SetMotionVectors(m_motionVectorPass->GetOutput());

        // 6. 执行管线
        m_pipeline->Execute(m_context);

        // 7. 保存用于下一帧
        m_prevViewProj = jitteredProj;
    }

    void SwitchTechnology(UpscalerTechnology tech) {
        if (m_upscalerPass->SetUpscaler(tech)) {
            std::cout << "Switched to " << UpscalerHelper::GetTechnologyName(tech) << std::endl;
        } else {
            std::cerr << "Failed to switch" << std::endl;
        }
    }

    void SetQuality(UpscalerQuality quality) {
        m_upscalerPass->SetQualityMode(quality);
    }

private:
    PrismaMath::mat4 ApplyJitter(const PrismaMath::mat4& proj,
                                  float jitterX, float jitterY,
                                  uint32_t width, uint32_t height) {
        PrismaMath::mat4 result = proj;
        result[2][0] += jitterX / width;
        result[2][1] += jitterY / height;
        return result;
    }
};
```

### GUI 集成（ImGui）

```cpp
void RenderUpscalerUI() {
    auto& manager = UpscalerManager::Instance();

    ImGui::Begin("Upscaler Settings");

    // 技术选择
    auto available = manager.GetAvailableTechnologies();
    auto current = manager.GetActiveUpscaler()->GetInfo().technology;

    if (ImGui::BeginCombo("Technology", UpscalerHelper::GetTechnologyName(current).c_str())) {
        for (auto tech : available) {
            bool isSelected = (tech == current);
            if (ImGui::Selectable(UpscalerHelper::GetTechnologyName(tech).c_str(), isSelected)) {
                m_upscalerPass->SetUpscaler(tech);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // 质量模式
    auto upscaler = manager.GetActiveUpscaler();
    auto info = upscaler->GetInfo();
    auto currentQuality = upscaler->GetQualityMode();

    if (ImGui::BeginCombo("Quality", UpscalerHelper::GetQualityName(currentQuality).c_str())) {
        for (auto quality : info.supportedQualities) {
            bool isSelected = (quality == currentQuality);
            if (ImGui::Selectable(UpscalerHelper::GetQualityName(quality).c_str(), isSelected)) {
                m_upscalerPass->SetQualityMode(quality);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // 信息显示
    uint32_t renderWidth, renderHeight;
    upscaler->GetRecommendedRenderResolution(currentQuality, 1920, 1080, renderWidth, renderHeight);

    ImGui::Text("Render Resolution: %dx%d", renderWidth, renderHeight);
    ImGui::Text("Display Resolution: 1920x1080");

    // 性能统计
    auto stats = upscaler->GetPerformanceStats();
    ImGui::Text("Avg Frame Time: %.2f ms", stats.avgFrameTime);
    ImGui::Text("Avg Upscale Time: %.2f ms", stats.avgUpscaleTime);

    ImGui::End();
}
```

---

## 最佳实践

### 1. 初始化顺序

```cpp
// 正确的初始化顺序
1. 创建渲染设备
2. 初始化 UpscalerManager
3. 创建并配置 UpscalerPass
4. 集成到渲染管线
5. 设置输入资源
```

### 2. 资源生命周期

```cpp
// 确保资源在超分器之前释放
~Renderer() {
    m_upscalerPass.reset();        // 1. 释放 Pass
    m_motionVectorPass.reset();     // 2. 释放运动矢量 Pass
    UpscalerManager::Instance().Shutdown();  // 3. 关闭管理器
    m_gbuffer.reset();              // 4. 释放 GBuffer
}
```

### 3. 错误处理

```cpp
// 总是检查返回值
if (!upscaler->Initialize(desc)) {
    // 回退到其他技术或禁用超分
    SetUpscaler(UpscalerTechnology::TSR);
}

if (!upscaler->Upscale(context, input, output)) {
    // 回退到原始渲染
    Blit(colorInput, outputTarget);
}
```

### 4. 性能优化

```cpp
// 使用性能模式进行快速预览
if (m_previewMode) {
    upscaler->SetQualityMode(UpscalerQuality::UltraPerformance);
} else {
    upscaler->SetQualityMode(m_savedQuality);
}

// 根据帧率动态调整
if (avgFrameTime > 16.6f) {  // 低于 60 FPS
    upscaler->SetQualityMode(UpscalerQuality::Performance);
}
```

---

## 参考文档

- [架构文档](UpscalerArchitecture.md)
- [渲染系统](RenderingSystem.md)
- [AMD FSR 文档](https://gpuopen.com/fidelityfx-superresolution/)
- [NVIDIA DLSS 文档](https://developer.nvidia.com/dlss)
