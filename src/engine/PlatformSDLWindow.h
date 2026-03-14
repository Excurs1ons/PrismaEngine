#pragma once

#include "Window.h"
#include <SDL3/SDL.h>

namespace Prisma {

class PlatformSDLWindow : public Window {
public:
    PlatformSDLWindow(const WindowProps& props);
    ~PlatformSDLWindow() override;

    void OnUpdate() override;

    uint32_t GetWidth() const override { return m_Data.Width; }
    uint32_t GetHeight() const override { return m_Data.Height; }

    void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
    void SetVSync(bool enabled) override;
    bool IsVSync() const override;

    void* GetNativeWindow() const override { return m_Window; }

private:
    virtual void Init(const WindowProps& props);
    virtual void Shutdown();

    SDL_Window* m_Window;

    struct WindowData {
        std::string Title;
        uint32_t Width, Height;
        bool VSync;
        EventCallbackFn EventCallback;
    };

    WindowData m_Data;
};

} // namespace Prisma
