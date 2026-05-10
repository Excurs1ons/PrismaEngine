#pragma once

#include "Export.h"
#include "core/Event.h"
#include "Platform.h"
#include <string>
#include <functional>
#include <memory>
#include <SDL3/SDL_video.h>

namespace Prisma {

/**
 * @brief 窗口抽象接口
 */
class ENGINE_API Window {
public:
    using EventCallbackFn = std::function<void(Event&)>;
    SDL_Window* m_Window;
    ~Window();
    void Init(const WindowProps& props);
    void Shutdown();
    virtual void OnUpdate();

    virtual uint32_t GetWidth() const { return m_Data.Width; }
    virtual uint32_t GetHeight() const { return m_Data.Height; }

    virtual void SetTitle(const std::string& title);

    // 窗口属性
    virtual void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }
    virtual void SetVSync(bool enabled);
    virtual bool IsVSync() const;

    virtual void* GetNativeWindow() const { return m_Window; }

    static std::unique_ptr<Window> Create(const WindowProps& props = WindowProps());
    struct WindowData {
        std::string Title;
        uint32_t Width, Height;
        bool VSync;
        EventCallbackFn EventCallback;
    };

    WindowData m_Data;
};

} // namespace Prisma
