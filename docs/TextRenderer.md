# TextRenderer 组件使用指南

## 概述

TextRenderer 组件提供了一个基于 TTF 字体的文本渲染系统，支持：
- TrueType 字体加载
- 动态位图字体图集生成
- UTF-8 文本渲染（支持 ASCII 和中文）
- 多行文本
- 可变颜色和大小

## 基本使用

### 1. 添加 TextRendererComponent 到 GameObject

```cpp
#include "graphic/ui/TextRendererComponent.h"

// 创建游戏对象并添加文本组件
GameObject* textObj = CreateGameObject();
auto textRenderer = AddComponent<TextRendererComponent>(textObj);

// 设置文本内容
textRenderer->SetText("Hello World!");

// 设置颜色（白色）
textRenderer->SetColor(PrismaMath::vec4(1.0f, 1.0f, 1.0f, 1.0f));

// 设置字体（可选，默认使用 "fonts/default.ttf"）
textRenderer->SetFont("fonts/Arial.ttf", 32.0f);
```

### 2. 设置位置

```cpp
// 获取 Transform 组件设置位置
auto transform = textObj->GetComponent<TransformComponent>();
transform->SetPosition(100.0f, 100.0f, 0.0f);  // 屏幕坐标 (x, y, z)
```

### 3. 注册到 UIPass

UIPass 会自动收集所有 TextRendererComponent 并渲染。确保 ForwardPipeline 已初始化：

```cpp
// ForwardPipeline 在渲染系统初始化时会自动创建 UIPass
// 无需手动注册
```

## API 参考

### TextRendererComponent

| 方法 | 描述 |
|------|------|
| `SetText(const std::string& text)` | 设置文本内容（UTF-8） |
| `SetFont(const std::string& path, float size)` | 设置字体文件和大小 |
| `SetColor(const PrismaMath::vec4& color)` | 设置文本颜色 (R, G, B, A) |
| `GetText()` | 获取当前文本内容 |
| `GetColor()` | 获取当前颜色 |
| `GetTextWidth()` | 获取文本宽度（像素） |
| `GetTextHeight()` | 获取文本高度（像素） |

### FontAtlas

字体图集类，通常由 TextRendererComponent 自动管理。

| 方法 | 描述 |
|------|------|
| `LoadFromTTF(path, size, ranges)` | 从 TTF 文件加载字体 |
| `GetGlyph(codepoint)` | 获取指定 Unicode 码点的字形信息 |
| `GetTexture()` | 获取字体图集纹理 |

### UIPass

UI 渲染通道，由 ForwardPipeline 自动管理。

| 方法 | 描述 |
|------|------|
| `AddText(component, transform)` | 添加文本到渲染队列 |
| `ClearQueue()` | 清空渲染队列 |
| `SetViewport(width, height)` | 设置视口大小 |

## 字体文件

字体文件应放在 `assets/fonts/` 或项目的资源目录下。支持的字体格式：
- `.ttf` - TrueType 字体
- `.otf` - OpenType 字体（部分支持）

## 字符范围

默认支持以下字符范围：
- ASCII: `0 - 127`
- CJK 统一汉字: `0x4E00 - 0x9FFF`

如需自定义字符范围，修改 `TextRendererComponent::s_charRanges` 数组。

## 着色器

文本渲染使用 `assets/shaders/Text.hlsl` 着色器，包含：
- 顶点着色器：位置变换和 UV 传递
- 像素着色器：字体纹理采样和颜色混合

## 注意事项

1. **性能考虑**：每个不同字体大小的组合会创建独立的字体图集
2. **内存管理**：字体图集默认为 2048x2048 RGBA8，约占用 16MB VRAM
3. **UTF-8 支持**：确保文本字符串是有效的 UTF-8 编码
4. **多行文本**：使用 `\n` 实现换行

## 示例代码

```cpp
// 创建 FPS 计数器
class FPSCounter : public Component {
private:
    TextRendererComponent* m_text;
    float m_accumulatedTime = 0.0f;
    int m_frameCount = 0;

public:
    void Initialize() override {
        m_text = gameObject()->GetComponent<TextRendererComponent>();
    }

    void Update(float deltaTime) override {
        m_accumulatedTime += deltaTime;
        m_frameCount++;

        if (m_accumulatedTime >= 1.0f) {
            int fps = m_frameCount;
            m_text->SetText("FPS: " + std::to_string(fps));
            m_frameCount = 0;
            m_accumulatedTime = 0.0f;
        }
    }
};

// 创建并设置 FPS 计数器
GameObject* fpsObj = CreateGameObject();
fpsObj->AddComponent<TransformComponent>()->SetPosition(10.0f, 10.0f, 0.0f);
auto text = fpsObj->AddComponent<TextRendererComponent>();
text->SetText("FPS: 60");
text->SetColor(PrismaMath::vec4(0.0f, 1.0f, 0.0f, 1.0f));  // 绿色
fpsObj->AddComponent<FPSCounter>();
```
