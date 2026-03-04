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

class RendererOpenGL :public RendererAPI {
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

private:
    void updateRenderArea();
    void createModels();

    ANativeWindow *window_;
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

public:
    void setAssetManager(AAssetManager *assetManager) {
        assetManager_ = assetManager;
    }
};

#endif //MY_APPLICATION_RENDEREROPENGL_H