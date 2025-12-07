#pragma once
#include "Component.h"
#include "RenderCommandContext.h"

// 渲染器组件基类
class RendererComponent : public Component {
public:
    virtual void Render(RenderCommandContext* context);

public:
    virtual void Update(float deltaTime) override;
    virtual ~RendererComponent() override;
    virtual void Initialize() override;
    virtual void Shutdown() override;
};
