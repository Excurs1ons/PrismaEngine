#pragma once
#include "Component.h"

#include <DirectXMath.h>
using namespace DirectX;
class Camera : public Component {
public:
    // 设置和获取清除颜色
    void SetClearColor(float r, float g, float b, float a = 1.0f) { m_clearColor = XMVectorSet(r, g, b, a); }
    XMVECTOR GetClearColor() { return m_clearColor; }

protected:
    // 清除颜色，默认为青色
    XMVECTOR m_clearColor;
};

