#ifndef MY_APPLICATION_RENDEREROPENGL_H
#define MY_APPLICATION_RENDEREROPENGL_H

#include "RendererAPI.h"
#include "Model.h"
#include "ShaderOpenGL.h"
#include <EGL/egl.h>
#include <memory>
#include <vector>
#include "Platform.h"
#include "graphic/interfaces/RenderTypes.h"
using namespace PrismaEngine::Graphic;

struct ANativeWindow;
struct AAssetManager;

class RendererOpenGL : public RendererAPI {
public:
    RendererOpenGL(ANativeWindow *window);
    ~RendererOpenGL() override;

    [[nodiscard]] std::string GetName() const;
    void init() override;
    void onConfigChanged() override;
    void render() override;
    void handleInput() override;

    // Native window 生命周期
    void onNativeWindowCreated(ANativeWindow *window) override;
    void onNativeWindowChanged(ANativeWindow *window) override;
    void onNativeWindowDestroyed() override;
    void onResume() override;
    void onPause() override;

    // 输入处理
    void onKeyDown(int keyCode) override;
    void onKeyUp(int keyCode) override;
    void onTouchEvent(int action, float x, float y) override;

    bool Initialize(const DeviceDesc& desc);
    void Render();

    // 设置资源管理器和内容区域（从 Java 端调用）
    void setAssetManager(AAssetManager *assetManager) override {
        assetManager_ = assetManager;
    }

    void setContentRect(int32_t left, int32_t top, int32_t right, int32_t bottom) override {
        contentRect_.left = left;
        contentRect_.top = top;
        contentRect_.right = right;
        contentRect_.bottom = bottom;
    }

private:
    void updateRenderArea();
    void createModels();

    ANativeWindow *window_ = nullptr;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLint width_;
    EGLint height_;

    bool shaderNeedsNewProjectionMatrix_;

    std::unique_ptr<ShaderOpenGL> shader_;
    std::vector<Model> models_;

    // Asset manager for loading resources
    AAssetManager *assetManager_ = nullptr;

    // Content rect
    struct ContentRect {
        int32_t left = 0;
        int32_t top = 0;
        int32_t right = 0;
        int32_t bottom = 0;
    } contentRect_;
};

#endif //MY_APPLICATION_RENDEREROPENGL_H
