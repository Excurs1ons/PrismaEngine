#pragma once
#include "Component.h"
#include "GameObject.h"

class Transform : public Component
{
public:
    // 添加简单的变换属性
    float position[3] = { 0.0f, 0.0f, 0.0f };
    float rotation[3] = { 0.0f, 0.0f, 0.0f };
    float scale[3] = { 1.0f, 1.0f, 1.0f };

    // 简单的获取矩阵方法
    float* GetMatrix() {
        // 返回单位矩阵作为占位符
        static float matrix[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
        return matrix;
    }
};