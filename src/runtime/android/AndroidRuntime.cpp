//
// Android Runtime (使用新的 GameActivity API)
// Android GameActivity 框架 - 与 native_app_glue 不同
//

#if defined(__ANDROID__) || defined(ANDROID)

#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>

#include "AndroidOut.h"
#include "Renderer.h"
#include "GameManager.h"

// 新的 game-activity API 头文件
#include <game-activity/GameActivity.h>
#include <game-text-input/gametextinput.h>

// 日志宏
#define LOG_TAG "PrismaEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 全局窗口和渲染器
static ANativeWindow *g_nativeWindow = nullptr;
static Renderer *g_renderer = nullptr;

// 输入管理
static GameTextInput *g_textInput = nullptr;

// ========== Native Window 处理 ==========

extern "C" void Java_com_example_myapplication_MainActivity_nativeOnSurfaceCreated(
    JNIEnv *env,
    jobject thiz,
    jobject surface) {

    LOGI("Surface created");

    if (surface != nullptr) {
        g_nativeWindow = ANativeWindow_fromSurface(env, surface);
        if (g_nativeWindow != nullptr && g_renderer == nullptr) {
            // 创建渲染器
            g_renderer = new Renderer();
            g_renderer->onNativeWindowCreated(g_nativeWindow);
            LOGI("Renderer created");
        }
    }
}

extern "C" void Java_com_example_myapplication_MainActivity_nativeOnSurfaceChanged(
    JNIEnv *env,
    jobject thiz,
    jobject surface,
    int width,
    int height) {

    LOGI("Surface changed: %dx%d", width, height);

    if (surface != nullptr) {
        ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
        if (g_renderer != nullptr) {
            g_renderer->onNativeWindowChanged(window);
            g_renderer->onConfigChanged();
        }
    }
}

extern "C" void Java_com_example_myapplication_MainActivity_nativeOnSurfaceDestroyed(
    JNIEnv *env,
    jobject thiz) {

    LOGI("Surface destroyed");

    if (g_renderer != nullptr) {
        g_renderer->onNativeWindowDestroyed();
    }

    if (g_nativeWindow != nullptr) {
        ANativeWindow_release(g_nativeWindow);
        g_nativeWindow = nullptr;
    }
}

// ========== 生命周期处理 ==========

extern "C" void Java_com_example_myapplication_MainActivity_nativeOnStart(
    JNIEnv *env,
    jobject thiz) {

    LOGI("Application started");
}

extern "C" void Java_com_example_myapplication_MainActivity_nativeOnResume(
    JNIEnv *env,
    jobject thiz) {

    LOGI("Application resumed");

    if (g_renderer != nullptr) {
        g_renderer->onResume();
    }
}

extern "C" void Java_com_example_myapplication_MainActivity_nativeOnPause(
    JNIEnv *env,
    jobject thiz) {

    LOGI("Application paused");

    if (g_renderer != nullptr) {
        g_renderer->onPause();
    }
}

extern "C" void Java_com_example_myapplication_MainActivity_nativeOnStop(
    JNIEnv *env,
    jobject thiz) {

    LOGI("Application stopped");

    if (g_renderer != nullptr) {
        g_renderer->onNativeWindowDestroyed();
    }

    if (g_renderer != nullptr) {
        delete g_renderer;
        g_renderer = nullptr;
    }
}

// ========== 输入处理 ==========

extern "C" void Java_com_example_myapplication_MainActivity_nativeOnKeyDown(
    JNIEnv *env,
    jobject thiz,
    int keyCode) {

    if (g_renderer != nullptr) {
        g_renderer->onKeyDown(keyCode);
    }
}

extern "C" void Java_com_example_myapplication_MainActivity_nativeOnKeyUp(
    JNIEnv *env,
    jobject thiz,
    int keyCode) {

    if (g_renderer != nullptr) {
        g_renderer->onKeyUp(keyCode);
    }
}

// ========== 触摸输入 ==========

extern "C" void Java_com_example_myapplication_MainActivity_nativeOnTouch(
    JNIEnv *env,
    jobject thiz,
    jint action,
    jfloat x,
    jfloat y) {

    if (g_renderer != nullptr) {
        g_renderer->onTouchEvent(action, x, y);
    }
}

// ========== 文本输入 ==========

extern "C" void Java_com_example_myapplication_MainActivity_nativeSetTextInput(
    JNIEnv *env,
    jobject thiz,
    jboolean show) {

    if (g_textInput == nullptr) {
        g_textInput = GameTextInput_getTextInput();
    }

    if (show) {
        GameTextInput_showSoftInput(g_textInput, 0);
    } else {
        GameTextInput_hideSoftInput(g_textInput);
    }
}

#endif // __ANDROID__
