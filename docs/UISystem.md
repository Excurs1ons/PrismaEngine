# PrismaEngine UI 系统开发路线

## 概述

PrismaEngine UI 系统是一个自主实现的游戏内 UI 框架，不支持 ImGui 等即时模式 GUI 库。系统设计支持 2D/3D 混合模式，基于事件驱动架构，与引擎 ECS 系统深度集成。

## 设计原则

1. **自主实现** - 不使用 ImGui，完全自研 UI 系统
2. **事件驱动** - 基于统一事件系统处理 UI 交互
3. **ECS 集成** - UI 元素继承 `Component`，与游戏对象系统统一
4. **双模式支持** - 2D UI（屏幕空间）和 3D UI（世界空间）
5. **平台抽象** - 平台层提供基础输入事件，UI 系统负责分发
6. **简单动画** - 枚举选择动画类型，仅需调整时间参数

## 当前状态

| 模块 | 完成度 | 状态 |
|------|--------|------|
| 事件系统 | 0% | 🔴 未开始 |
| 平台输入事件 | 30% | 🔴 需完善 |
| 2D UI 系统 | 0% | 🔴 未开始 |
| 3D UI 系统 | 0% | 🔴 未开始 |
| UI 渲染通道 | 20% | 🟡 框架已建立 |
| 物理系统 | 5% | 🔴 未开始 |

## 现有基础

### 已有代码
- `src/engine/graphic/ui/UIPass.h/cpp` - UI 渲染通道框架
- `src/engine/graphic/ui/TextRendererComponent.h/cpp` - 文本渲染组件
- `src/engine/input/InputManager.h/cpp` - 输入管理器（需扩展）
- `src/engine/Component.h` - 组件基类
- `src/engine/GameObject.h` - 游戏对象系统

### Component 基类接口
```cpp
class Component {
public:
    virtual ~Component() = default;
    virtual void Initialize(){}
    virtual void Update(float deltaTime) {}
    virtual void Shutdown(){}

    void SetOwner(GameObject* owner);
    GameObject* GetOwner() const;

protected:
    GameObject* owner = nullptr;
};
```

---

## 第一部分：UI 生命周期系统

### 1.1 生命周期阶段

```cpp
// UI 组件生命周期阶段
enum class UILifecyclePhase {
    // 初始化阶段
    Created,        // 组件创建后
    Initializing,   // Initialize() 调用中

    // 显示阶段（打开）
    BeforeShow,     // 显示前（动画开始前）
    Showing,        // 显示中（动画进行中）
    AfterShow,      // 显示后（动画完成）

    // 运行阶段
    Active,         // 激活状态

    // 交互阶段
    HoverEnter,     // 鼠标进入
    HoverLeave,     // 鼠标离开
    Pressed,        // 按下
    Released,       // 释放
    Clicked,        // 点击（按下+释放）

    // 隐藏阶段（关闭）
    BeforeHide,     // 隐藏前（动画开始前）
    Hiding,         // 隐藏中（动画进行中）
    AfterHide,      // 隐藏后（动画完成）

    // 销毁阶段
    BeforeDestroy,  // 销毁前
    Destroying      // 销毁中
};
```

### 1.2 生命周期回调接口

```cpp
// UI 生命周期回调接口
class IUILifecycleListener {
public:
    virtual ~IUILifecycleListener() = default;

    // 显示回调
    virtual void OnBeforeShow() {}
    virtual void OnAfterShow() {}

    // 隐藏回调
    virtual void OnBeforeHide() {}
    virtual void OnAfterHide() {}

    // 交互回调
    virtual void OnHoverEnter() {}
    virtual void OnHoverLeave() {}
    virtual void OnPressed() {}
    virtual void OnReleased() {}
    virtual void OnClicked() {}

    // 销毁回调
    virtual void OnBeforeDestroy() {}
};
```

---

## 第二部分：Tweeny 动画封装

### 2.1 动画类型枚举

```cpp
// UI 动画类型
enum class UIAnimationType {
    None,           // 无动画

    // 淡入淡出
    FadeIn,         // 淡入（透明度 0→1）
    FadeOut,        // 淡出（透明度 1→0）

    // 缩放动画
    ScaleUp,        // 放大（0→1）
    ScaleDown,      // 缩小（1→0）
    ScalePop,       // 弹出（0→1.2→1）

    // 滑动动画
    SlideInLeft,    // 从左滑入
    SlideInRight,   // 从右滑入
    SlideInTop,     // 从上滑入
    SlideInBottom,  // 从下滑入
    SlideOutLeft,   // 向左滑出
    SlideOutRight,  // 向右滑出
    SlideOutTop,    // 向上滑出
    SlideOutBottom, // 向下滑出

    // 旋转动画
    RotateIn,       // 旋转进入
    RotateOut,      // 旋转退出

    // 组合动画
    PopIn,          // 弹入（缩放+淡入）
    PopOut,         // 弹出（缩放+淡出）
};
```

### 2.2 缓动函数枚举

```cpp
// 缓动函数（映射自 tweeny::easing）
enum class UIEasingType {
    Linear,

    // Quad
    QuadIn, QuadOut, QuadInOut,

    // Cubic
    CubicIn, CubicOut, CubicInOut,

    // Quart
    QuartIn, QuartOut, QuartInOut,

    // Quint
    QuintIn, QuintOut, QuintInOut,

    // Sine
    SineIn, SineOut, SineInOut,

    // Expo
    ExpoIn, ExpoOut, ExpoInOut,

    // Circ
    CircIn, CircOut, CircInOut,

    // Back
    BackIn, BackOut, BackInOut,

    // Elastic
    ElasticIn, ElasticOut, ElasticInOut,

    // Bounce
    BounceIn, BounceOut, BounceInOut,
};
```

### 2.3 UI 动画封装器

```cpp
// src/engine/ui/UIAnimation.h
#pragma once

#include "tweeny.h"
#include <functional>
#include <memory>

namespace PrismaEngine {

class UIComponent; // 前向声明

// 动画封装器
class UIAnimation {
public:
    using Callback = std::function<void()>;

    UIAnimation(UIAnimationType type, float duration, UIEasingType easing = UIEasing::QuadInOut);

    // 步进动画
    bool Step(float deltaTime);

    // 是否完成
    bool IsCompleted() const { return m_completed; }

    // 获取当前值（用于渲染）
    float GetAlpha() const { return m_alpha; }
    float GetScale() const { return m_scale; }
    PrismaMath::vec2 GetOffset() const { return m_offset; }
    float GetRotation() const { return m_rotation; }

    // 回调设置
    void OnComplete(Callback cb) { m_onComplete = std::move(cb); }
    void OnStep(Callback cb) { m_onStep = std::move(cb); }

private:
    void CreateTween();

private:
    UIAnimationType m_type;
    float m_duration;
    UIEasingType m_easing;
    float m_elapsed = 0.0f;
    bool m_completed = false;
    bool m_reverse = false; // true = 反向（hide 动画）

    // 动画值
    float m_alpha = 1.0f;
    float m_scale = 1.0f;
    PrismaMath::vec2 m_offset{0.0f, 0.0f};
    float m_rotation = 0.0f;

    // Tweeny 实例
    std::variant<
        tweeny::tween<float, float>,
        tweeny::tween<float, float, float>
    > m_tween;

    Callback m_onComplete;
    Callback m_onStep;
};

} // namespace PrismaEngine
```

### 2.4 简化的动画配置

```cpp
// UI 组件动画配置
struct UIAnimationConfig {
    // 显示动画
    UIAnimationType showAnimation = UIAnimationType::FadeIn;
    float showDuration = 0.3f;
    UIEasingType showEasing = UIEasingType::QuadOut;

    // 隐藏动画
    UIAnimationType hideAnimation = UIAnimationType::FadeOut;
    float hideDuration = 0.2f;
    UIEasingType hideEasing = UIEasingType::QuadIn;

    // 悬停动画（可选）
    UIAnimationType hoverAnimation = UIAnimationType::ScaleUp;
    float hoverDuration = 0.15f;
    float hoverScale = 1.1f;

    // 点击动画（可选）
    UIAnimationType clickAnimation = UIAnimationType::ScalePop;
    float clickDuration = 0.1f;
    float clickScale = 0.95f;
};
```

---

## 第三部分：UI 组件基类

### 3.1 UIComponent 核心设计

```cpp
// src/engine/ui/UIComponent.h
#pragma once

#include "Component.h"
#include "UIAnimation.h"
#include "IUILifecycleListener.h"
#include "math/MathTypes.h"
#include <vector>
#include <memory>

namespace PrismaEngine {

class CanvasComponent;

// UI 组件基类（继承 Component）
class UIComponent : public Component, public IUILifecycleListener {
public:
    UIComponent();
    ~UIComponent() override;

    // Component 接口
    void Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;

    // === 生命周期控制 ===
    void Show();      // 显示组件（触发显示动画）
    void Hide();      // 隐藏组件（触发隐藏动画）
    bool IsVisible() const { return m_visible; }
    bool IsAnimating() const;

    // === 动画配置 ===
    void SetAnimationConfig(const UIAnimationConfig& config) { m_animConfig = config; }
    const UIAnimationConfig& GetAnimationConfig() const { return m_animConfig; }

    // === 变换属性 ===
    void SetPosition(const PrismaMath::vec2& pos) { m_position = pos; }
    void SetSize(const PrismaMath::vec2& size) { m_size = size; }
    void SetAnchor(const PrismaMath::vec2& anchor) { m_anchor = anchor; }
    void SetPivot(const PrismaMath::vec2& pivot) { m_pivot = pivot; }

    const PrismaMath::vec2& GetPosition() const { return m_position; }
    const PrismaMath::vec2& GetSize() const { return m_size; }
    const PrismaMath::vec2& GetAnchor() const { return m_anchor; }
    const PrismaMath::vec2& GetPivot() const { return m_pivot; }

    // 计算世界位置（考虑锚点、枢轴、动画偏移）
    PrismaMath::vec2 GetWorldPosition() const;
    PrismaMath::vec2 GetWorldScale() const;
    float GetWorldAlpha() const;

    // === 层级管理 ===
    void SetParent(UIComponent* parent);
    UIComponent* GetParent() const { return m_parent; }
    void AddChild(UIComponent* child);
    void RemoveChild(UIComponent* child);
    const std::vector<UIComponent*>& GetChildren() const { return m_children; }

    // === 交互属性 ===
    void SetInteractable(bool interactable) { m_interactable = interactable; }
    bool IsInteractable() const { return m_interactable && m_visible; }

    void SetZOrder(int z) { m_zOrder = z; }
    int GetZOrder() const { return m_zOrder; }

    // === 事件处理 ===
    virtual bool HandleEvent(const struct InputEvent& event);
    virtual void OnHoverEnter() override;
    virtual void OnHoverLeave() override;
    virtual void OnPressed() override;
    virtual void OnReleased() override;
    virtual void OnClicked() override;

    // === 命中测试 ===
    virtual bool HitTest(const PrismaMath::vec2& point) const;

protected:
    // 渲染回调（子类实现）
    virtual void Render(class RenderContext& ctx) {}

    // 生命周期回调
    virtual void OnBeforeShow() override;
    virtual void OnAfterShow() override;
    virtual void OnBeforeHide() override;
    virtual void OnAfterHide() override;

protected:
    // 显隐状态
    bool m_visible = true;
    bool m_interactable = true;

    // 变换
    PrismaMath::vec2 m_position{0.0f, 0.0f};
    PrismaMath::vec2 m_size{100.0f, 100.0f};
    PrismaMath::vec2 m_anchor{0.5f, 0.5f};  // 锚点（相对父级）
    PrismaMath::vec2 m_pivot{0.5f, 0.5f};   // 枢轴（相对自身）

    // 层级
    int m_zOrder = 0;
    UIComponent* m_parent = nullptr;
    std::vector<UIComponent*> m_children;

    // 动画
    UIAnimationConfig m_animConfig;
    std::unique_ptr<UIAnimation> m_currentAnimation;

    // 交互状态
    bool m_isHovered = false;
    bool m_isPressed = false;

    // 缓存的世界变换
    mutable PrismaMath::vec2 m_cachedWorldPos{0.0f, 0.0f};
    mutable bool m_worldTransformDirty = true;
};

} // namespace PrismaEngine
```

### 3.2 具体组件示例

```cpp
// src/engine/ui/2d/ButtonComponent.h
#pragma once

#include "UIComponent.h"
#include <string>
#include <functional>

namespace PrismaEngine {

// 按钮组件
class ButtonComponent : public UIComponent {
public:
    using ClickCallback = std::function<void()>;

    ButtonComponent();
    ~ButtonComponent() override;

    // Component 接口
    void Initialize() override;
    void Update(float deltaTime) override;

    // 文本设置
    void SetText(const std::string& text);
    const std::string& GetText() const { return m_text; }

    // 样式设置
    void SetNormalColor(const PrismaMath::vec4& color) { m_normalColor = color; }
    void SetHoverColor(const PrismaMath::vec4& color) { m_hoverColor = color; }
    void SetPressedColor(const PrismaMath::vec4& color) { m_pressedColor = color; }
    void SetDisabledColor(const PrismaMath::vec4& color) { m_disabledColor = color; }

    // 点击回调
    void SetOnClick(ClickCallback callback) { m_onClick = std::move(callback); }

    // 生命周期
    void OnHoverEnter() override;
    void OnHoverLeave() override;
    void OnPressed() override;
    void OnReleased() override;

protected:
    void Render(RenderContext& ctx) override;

private:
    PrismaMath::vec4 GetCurrentColor() const;

private:
    std::string m_text;
    PrismaMath::vec4 m_normalColor{1.0f, 1.0f, 1.0f, 1.0f};
    PrismaMath::vec4 m_hoverColor{0.9f, 0.9f, 0.9f, 1.0f};
    PrismaMath::vec4 m_pressedColor{0.7f, 0.7f, 0.7f, 1.0f};
    PrismaMath::vec4 m_disabledColor{0.5f, 0.5f, 0.5f, 0.5f};
    ClickCallback m_onClick;
};

// === 使用示例 ===
/*
// 创建按钮
auto button = gameObject->AddComponent<ButtonComponent>();
button->SetText("Start Game");
button->SetPosition({100, 100});
button->SetSize({200, 50});

// 配置动画（枚举 + 时间参数）
UIAnimationConfig animConfig;
animConfig.showAnimation = UIAnimationType::PopIn;
animConfig.showDuration = 0.4f;
animConfig.hoverAnimation = UIAnimationType::ScaleUp;
animConfig.hoverDuration = 0.15f;
animConfig.clickAnimation = UIAnimationType::ScalePop;
animConfig.clickDuration = 0.1f;
button->SetAnimationConfig(animConfig);

// 设置点击回调
button->SetOnClick([]() {
    LOG("Button clicked!");
});

// 显示（带动画）
button->Show();
*/

} // namespace PrismaEngine
```

```cpp
// src/engine/ui/2d/ImageComponent.h
#pragma once

#include "UIComponent.h"
#include <string>

namespace PrismaEngine {

// 图片组件
class ImageComponent : public UIComponent {
public:
    ImageComponent();
    ~ImageComponent() override;

    void Initialize() override;
    void Update(float deltaTime) override;

    // 图片设置
    void SetTexture(const std::string& texturePath);
    void SetTexture(class TextureHandle texture);
    void SetColor(const PrismaMath::vec4& color) { m_color = color; }

    // 图片模式
    enum class ImageMode {
        Simple,     // 简单拉伸
        Sliced,     // 九宫格切片
        Tiled,      // 平铺
        Filled,     // 填充（用于进度条等）
    };
    void SetImageMode(ImageMode mode) { m_imageMode = mode; }

    // 填充类型（Filled 模式）
    enum class FillType {
        Horizontal, // 水平填充
        Vertical,   // 垂直填充
        Radial90,   // 90度径向
        Radial180,  // 180度径向
        Radial360,  // 360度径向
    };
    void SetFillType(FillType type) { m_fillType = type; }
    void SetFillAmount(float amount) { m_fillAmount = amount; } // 0.0 - 1.0

protected:
    void Render(RenderContext& ctx) override;

private:
    TextureHandle m_texture;
    PrismaMath::vec4 m_color{1.0f, 1.0f, 1.0f, 1.0f};
    ImageMode m_imageMode = ImageMode::Simple;
    FillType m_fillType = FillType::Horizontal;
    float m_fillAmount = 1.0f;

    // 九宫格边框
    PrismaMath::vec4 m_border{0.0f, 0.0f, 0.0f, 0.0f}; // left, bottom, right, top
};

} // namespace PrismaEngine
```

```cpp
// src/engine/ui/2d/SliderComponent.h
#pragma once

#include "UIComponent.h"
#include "ImageComponent.h"
#include <functional>

namespace PrismaEngine {

// 滑块组件
class SliderComponent : public UIComponent {
public:
    using ValueChangeCallback = std::function<void(float)>;

    SliderComponent();
    ~SliderComponent() override;

    void Initialize() override;
    void Update(float deltaTime) override;

    // 值设置
    void SetValue(float value);
    float GetValue() const { return m_value; }

    void SetMinValue(float min) { m_minValue = min; }
    void SetMaxValue(float max) { m_maxValue = max; }

    void SetWholeNumbers(bool whole) { m_wholeNumbers = whole; }

    // 回调
    void SetOnValueChanged(ValueChangeCallback callback) { m_onValueChanged = std::move(callback); }

    // 样式
    void SetBackground(ImageComponent* bg) { m_background = bg; }
    void SetFill(ImageComponent* fill) { m_fill = fill; }
    void SetHandle(ImageComponent* handle) { m_handle = handle; }

protected:
    void Render(RenderContext& ctx) override;
    bool HandleEvent(const InputEvent& event) override;

private:
    void UpdateLayout();

private:
    float m_value = 0.5f;
    float m_minValue = 0.0f;
    float m_maxValue = 1.0f;
    bool m_wholeNumbers = false;

    // 子组件
    ImageComponent* m_background = nullptr;
    ImageComponent* m_fill = nullptr;
    ImageComponent* m_handle = nullptr;

    ValueChangeCallback m_onValueChanged;
};

} // namespace PrismaEngine
```

---

## 第四部分：Canvas 容器

```cpp
// src/engine/ui/CanvasComponent.h
#pragma once

#include "UIComponent.h"
#include "UICamera.h"
#include <memory>

namespace PrismaEngine {

// 渲染模式
enum class CanvasRenderMode {
    ScreenSpace,       // 屏幕空间（2D UI）
    ScreenSpaceCamera, // 屏幕空间-相机（带透视）
    WorldSpace,        // 世界空间（3D UI，需物理系统）
};

// 画布组件（UI 根容器）
class CanvasComponent : public UIComponent {
public:
    CanvasComponent();
    ~CanvasComponent() override;

    void Initialize() override;
    void Update(float deltaTime) override;

    // 渲染模式
    void SetRenderMode(CanvasRenderMode mode) { m_renderMode = mode; }
    CanvasRenderMode GetRenderMode() const { return m_renderMode; }

    // 相机设置（ScreenSpaceCamera 模式）
    void SetCamera(UICamera* camera) { m_camera = camera; }

    // 渲染尺寸（ScreenSpace 模式）
    void SetRenderResolution(const PrismaMath::vec2& resolution) { m_resolution = resolution; }

    // 排序（按 ZOrder 重排子元素）
    void SortChildren();

    // 渲染（由 UIPass 调用）
    void Render(class RenderContext& ctx);

private:
    CanvasRenderMode m_renderMode = CanvasRenderMode::ScreenSpace;
    UICamera* m_camera = nullptr;
    PrismaMath::vec2 m_resolution{1920.0f, 1080.0f};
};

} // namespace PrismaEngine
```

---

## 开发路线

### 阶段 0：平台输入事件完善

**目标**：完善平台层，为事件系统提供可靠的输入数据源

**任务清单**：
- [ ] 扩展 `InputManager` 支持更多输入类型
- [ ] 实现输入事件缓存机制
- [ ] 添加触摸事件支持（移动端关键）
- [ ] 统一键盘/鼠标/触摸事件格式

**文件规划**：
```
src/engine/input/
├── InputEvent.h           # 输入事件定义
├── InputEventQueue.h      # 事件队列
└── InputType.h            # 输入类型枚举
```

**依赖**：无
**优先级**：🔴 高

---

### 阶段 1：事件系统建立

**目标**：构建统一的事件总线，支持事件发布/订阅机制

**任务清单**：
- [ ] 实现 `Event` 基类和事件类型系统
- [ ] 实现 `EventDispatcher` 事件分发器
- [ ] 实现输入事件到系统事件的转换
- [ ] 添加事件过滤和优先级机制
- [ ] 实现事件冒泡/捕获机制

**文件规划**：
```
src/engine/event/
├── Event.h                # 事件基类
├── EventDispatcher.h      # 事件分发器
├── InputEvent.h           # 输入事件
├── MouseEvent.h           # 鼠标事件
├── KeyboardEvent.h        # 键盘事件
├── TouchEvent.h           # 触摸事件
└── UIEvent.h              # UI 事件
```

**依赖**：阶段 0
**优先级**：🔴 高

---

### 阶段 2：Tweeny 集成

**目标**：封装 Tweeny 库，提供简单的动画 API

**任务清单**：
- [ ] 添加 [Tweeny](https://github.com/mobius3/tweeny) 子模块
- [ ] 实现 `UIAnimation` 封装类
- [ ] 实现 `UIAnimationType` 枚举到 tweeny 的映射
- [ ] 实现 `UIEasingType` 枚举到 tweeny::easing 的映射
- [ ] 实现动画值计算（alpha, scale, offset, rotation）
- [ ] 添加动画回调支持

**文件规划**：
```
src/engine/ui/animation/
├── UIAnimation.h          # 动画封装
├── UIAnimation.cpp
├── AnimationType.h        # 枚举定义
└── EasingMapping.h        # Easing 映射
```

**依赖**：无
**优先级**：🟡 中高

---

### 阶段 3：UI 组件基类

**目标**：实现 `UIComponent` 基类和生命周期系统

**任务清单**：
- [ ] 实现 `UIComponent` 基类（继承 `Component`）
- [ ] 实现生命周期状态机
- [ ] 实现 `Show()` / `Hide()` 带动画
- [ ] 实现变换系统（position, size, anchor, pivot）
- [ ] 实现层级管理（parent/children）
- [ ] 实现命中测试（HitTest）
- [ ] 实现交互状态管理

**文件规划**：
```
src/engine/ui/
├── UIComponent.h          # UI 组件基类
├── UIComponent.cpp
└── UILifecycle.h          # 生命周期定义
```

**依赖**：阶段 2
**优先级**：🟡 中高

---

### 阶段 4：基础 UI 组件

**目标**：实现常见 UI 控件

**任务清单**：
- [ ] `CanvasComponent` - 画布容器
- [ ] `ButtonComponent` - 按钮
- [ ] `ImageComponent` - 图片
- [ ] `TextComponent` - 文本（扩展现有 `TextRendererComponent`）
- [ ] `SliderComponent` - 滑块
- [ ] `ToggleComponent` - 开关

**文件规划**：
```
src/engine/ui/2d/
├── CanvasComponent.h
├── ButtonComponent.h
├── ImageComponent.h
├── TextComponent.h
├── SliderComponent.h
└── ToggleComponent.h
```

**依赖**：阶段 3
**优先级**：🟡 中

---

### 阶段 5：UI 交互系统

**目标**：实现完整的 UI 交互逻辑

**任务清单**：
- [ ] 实现点击检测
- [ ] 实现悬停状态管理
- [ ] 实现拖拽系统
- [ ] 实现焦点系统（键盘导航）
- [ ] 实现事件冒泡和捕获

**文件规划**：
```
src/engine/ui/interaction/
├── HitTest.h              # 点击检测
├── FocusManager.h         # 焦点管理
├── DragDrop.h             # 拖拽系统
└── EventRouting.h         # 事件路由
```

**依赖**：阶段 4
**优先级**：🟡 中

---

### 阶段 6：布局系统

**目标**：实现自动布局

**任务清单**：
- [ ] 实现锚点系统
- [ ] 实现自适应布局
- [ ] 实现流式布局
- [ ] 实现网格布局
- [ ] 实现滚动视图

**文件规划**：
```
src/engine/ui/layout/
├── AnchorLayout.h         # 锚点布局
├── FlexLayout.h           # 流式布局
├── GridLayout.h           # 网格布局
└── ScrollView.h           # 滚动视图
```

**依赖**：阶段 4
**优先级**：🟢 中低

---

### 阶段 7：3D UI 系统（暂缓）

**前置条件**：物理系统完成

**目标**：支持世界空间中的 3D UI

**任务清单**：
- [ ] 实现物理射线检测
- [ ] 实现 3D UI 坐标转换
- [ ] 实现 `WorldSpaceCanvas` 组件
- [ ] 实现 3D 事件投射

**依赖**：物理系统
**优先级**：⚪ 低（远期）

---

## 目录结构总览

```
src/engine/
├── event/                         # 新建 - 事件系统
│   ├── Event.h
│   ├── EventDispatcher.h
│   ├── InputEvent.h
│   ├── MouseEvent.h
│   ├── KeyboardEvent.h
│   ├── TouchEvent.h
│   └── UIEvent.h
│
├── ui/                            # 新建 - UI 系统
│   ├── UIComponent.h              # UI 组件基类（继承 Component）
│   ├── UIComponent.cpp
│   ├── CanvasComponent.h          # 画布容器
│   ├── UICamera.h                 # UI 相机
│   ├── UICursor.h                 # 光标管理
│   ├── UILifecycle.h              # 生命周期定义
│   │
│   ├── animation/                 # 动画系统
│   │   ├── UIAnimation.h          # Tweeny 封装
│   │   ├── UIAnimation.cpp
│   │   ├── AnimationType.h        # 动画类型枚举
│   │   └── EasingMapping.h        # Easing 映射
│   │
│   ├── 2d/                        # 2D UI 组件
│   │   ├── ButtonComponent.h      # 按钮
│   │   ├── ButtonComponent.cpp
│   │   ├── ImageComponent.h       # 图片
│   │   ├── ImageComponent.cpp
│   │   ├── TextComponent.h        # 文本
│   │   ├── TextComponent.cpp
│   │   ├── SliderComponent.h      # 滑块
│   │   ├── SliderComponent.cpp
│   │   └── ToggleComponent.h      # 开关
│   │
│   ├── 3d/                        # 3D UI 组件（暂缓）
│   │   └── WorldSpaceCanvas.h
│   │
│   ├── layout/                    # 布局系统
│   │   ├── AnchorLayout.h
│   │   ├── FlexLayout.h
│   │   ├── GridLayout.h
│   │   └── ScrollView.h
│   │
│   └── interaction/               # 交互系统
│       ├── HitTest.h
│       ├── FocusManager.h
│       ├── DragDrop.h
│       └── EventRouting.h
│
└── input/                         # 扩展现有
    ├── InputEvent.h               # 统一输入事件
    └── InputEventQueue.h          # 事件队列

external/
└── tweeny/                        # Tweeny 库（git submodule）
    └── tweeny.h
```

---

## 使用示例

```cpp
// 创建 Canvas
auto canvasObj = GameObject::Create("Canvas");
auto canvas = canvasObj->AddComponent<CanvasComponent>();
canvas->SetRenderMode(CanvasRenderMode::ScreenSpace);

// 创建按钮
auto buttonObj = GameObject::Create("StartButton");
buttonObj->SetParent(canvasObj);
auto button = buttonObj->AddComponent<ButtonComponent>();

// 设置布局
button->SetPosition({0.0f, 100.0f});  // 屏幕中心向上 100
button->SetSize({200.0f, 50.0f});
button->SetAnchor({0.5f, 0.5f});     // 中心锚点

// 设置文本
button->SetText("Start Game");

// 配置动画（仅需枚举 + 时间）
UIAnimationConfig anim;
anim.showAnimation = UIAnimationType::PopIn;
anim.showDuration = 0.4f;
anim.hoverAnimation = UIAnimationType::ScaleUp;
anim.hoverDuration = 0.15f;
anim.clickAnimation = UIAnimationType::ScalePop;
anim.clickDuration = 0.1f;
button->SetAnimationConfig(anim);

// 设置点击回调
button->SetOnClick([]() {
    SceneManager::LoadScene("GameScene");
});

// 显示（带动画）
button->Show();

// 创建滑块
auto sliderObj = GameObject::Create("VolumeSlider");
sliderObj->SetParent(canvasObj);
auto slider = sliderObj->AddComponent<SliderComponent>();
slider->SetPosition({0.0f, -50.0f});
slider->SetSize({300.0f, 20.0f});
slider->SetValue(0.7f);

// 滑块值变化回调
slider->SetOnValueChanged([](float value) {
    AudioManager::SetMasterVolume(value);
});
```

---

## 相关库

- [Tweeny](https://github.com/mobius3/tweeny) - 补间动画库（header-only，零依赖）
- [msdfgen](https://github.com/Chlumsky/msdfgen) - 多距离场字体生成
- [JoltPhysics](https://github.com/jrouwe/JoltPhysics) - 物理引擎（3D UI 需要）

---

*创建日期: 2026-01-03*
*最后更新: 2026-01-03*
