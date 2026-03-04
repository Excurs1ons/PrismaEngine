#ifndef MY_APPLICATION_RENDERERAPI_H
#define MY_APPLICATION_RENDERERAPI_H

struct ANativeWindow;

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
};

#endif //MY_APPLICATION_RENDERERAPI_H
