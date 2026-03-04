#ifndef MY_APPLICATION_RENDERERAPI_H
#define MY_APPLICATION_RENDERERAPI_H

#include <cstdint>

struct ANativeWindow;
struct AAssetManager;

class RendererAPI {
public:
    virtual ~RendererAPI() = default;
    virtual void init() = 0;
    virtual void render() = 0;
    virtual void onConfigChanged() = 0;  // 处理屏幕旋转等配置变化
    virtual void handleInput() = 0;      // 处理输入（包括 UI 交互）

    // Native window 生命周期方法
    virtual void onNativeWindowCreated(ANativeWindow *window) = 0;
    virtual void onNativeWindowChanged(ANativeWindow *window) = 0;
    virtual void onNativeWindowDestroyed() = 0;
    virtual void onResume() = 0;
    virtual void onPause() = 0;

    // 输入处理方法
    virtual void onKeyDown(int keyCode) = 0;
    virtual void onKeyUp(int keyCode) = 0;
    virtual void onTouchEvent(int action, float x, float y) = 0;

    // 资源和内容区域设置方法（默认实现为空，子类可以覆盖）
    virtual void setAssetManager(AAssetManager *assetManager) { (void)assetManager; }
    virtual void setContentRect(int32_t left, int32_t top, int32_t right, int32_t bottom) {
        (void)left; (void)top; (void)right; (void)bottom;
    }
};

#endif //MY_APPLICATION_RENDERERAPI_H
