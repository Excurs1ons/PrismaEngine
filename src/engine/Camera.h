#pragma once
#include "Component.h"
#include "graphic/ICamera.h"
#include "math/MathTypes.h"

class Camera : public Component, public Engine::Graphic::ICamera {
public:
    // 设置和获取清除颜色
    void SetClearColor(float r, float g, float b, float a = 1.0f) { m_clearColor = PrismaMath::vec4(r, g, b, a); }
    PrismaMath::vec4 GetClearColor() { return m_clearColor; }

protected:
    // 清除颜色，默认为青色
    PrismaMath::vec4 m_clearColor;
};

