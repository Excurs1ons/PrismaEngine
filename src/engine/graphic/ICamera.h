#pragma once

#include <DirectXMath.h>
using namespace DirectX;

namespace Engine {
namespace Graphic {

// 相机接口
class ICamera {
public:
    virtual ~ICamera() = default;

    // 获取视图矩阵
    virtual XMMATRIX GetViewMatrix() const = 0;

    // 获取投影矩阵
    virtual XMMATRIX GetProjectionMatrix() const = 0;

    // 获取视图投影矩阵
    virtual XMMATRIX GetViewProjectionMatrix() const = 0;

    // 获取相机位置
    virtual XMVECTOR GetPosition() const = 0;

    // 获取相机朝向
    virtual XMVECTOR GetForward() const = 0;

    // 获取相机上方向
    virtual XMVECTOR GetUp() const = 0;

    // 获取相机右方向
    virtual XMVECTOR GetRight() const = 0;

    // 获取视野角度（弧度）
    virtual float GetFOV() const = 0;

    // 获取近平面
    virtual float GetNearPlane() const = 0;

    // 获取远平面
    virtual float GetFarPlane() const = 0;

    // 获取宽高比
    virtual float GetAspectRatio() const = 0;

    // 设置视野角度
    virtual void SetFOV(float fov) = 0;

    // 设置近平面和远平面
    virtual void SetNearFarPlanes(float nearPlane, float farPlane) = 0;

    // 设置宽高比
    virtual void SetAspectRatio(float aspectRatio) = 0;

    // 更新相机（每帧调用）
    virtual void Update(float deltaTime) = 0;

    // 是否是活动相机
    virtual bool IsActive() const = 0;

    // 设置活动状态
    virtual void SetActive(bool active) = 0;

    // 获取清除颜色
    virtual DirectX::XMVECTOR GetClearColor() const = 0;

    // 设置清除颜色
    virtual void SetClearColor(float r, float g, float b, float a = 1.0f) = 0;
};

} // namespace Graphic
} // namespace Engine