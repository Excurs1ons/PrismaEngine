#ifndef ANDROIDGLINVESTIGATIONS_RENDERER_H
#define ANDROIDGLINVESTIGATIONS_RENDERER_H

#include "RendererAPI.h"
#include <memory>

struct ANativeWindow;

class Renderer {
public:
    /*!
     * @param window the native window for rendering
     */
    Renderer(ANativeWindow *window = nullptr);

    virtual ~Renderer();

    /*!
     * Handles input from the android_app.
     *
     * Note: this will clear the input queue
     */
    void handleInput();

    /*!
     * Renders all the models in the renderer
     */
    void render();

    /*!
     * Handles configuration changes (e.g., screen rotation)
     */
    void onConfigChanged();

    /*!
     * Called when the native window is created
     */
    void onNativeWindowCreated(ANativeWindow *window);

    /*!
     * Called when the native window is changed (e.g., after rotation)
     */
    void onNativeWindowChanged(ANativeWindow *window);

    /*!
     * Called when the native window is about to be destroyed
     */
    void onNativeWindowDestroyed();

    /*!
     * Called when the app is resumed
     */
    void onResume();

    /*!
     * Called when the app is paused
     */
    void onPause();

    /*!
     * Handle key down event
     */
    void onKeyDown(int keyCode);

    /*!
     * Handle key up event
     */
    void onKeyUp(int keyCode);

    /*!
     * Handle touch event
     */
    void onTouchEvent(int action, float x, float y);

    /*!
     * Set the asset manager for loading resources
     */
    void setAssetManager(void *assetManager);

    /*!
     * Set the content rect (for handling status bar, notches, etc.)
     */
    void setContentRect(int32_t left, int32_t top, int32_t right, int32_t bottom);

private:
    ANativeWindow *window_;
    std::unique_ptr<RendererAPI> impl_;
};

#endif //ANDROIDGLINVESTIGATIONS_RENDERER_H
