#include "PlatformSDLWindow.h"
#include "Logger.h"
#include "core/Event.h"

namespace Prisma {

PlatformSDLWindow::PlatformSDLWindow(const WindowProps& props) {
    Init(props);
}

PlatformSDLWindow::~PlatformSDLWindow() {
    Shutdown();
}

void PlatformSDLWindow::Init(const WindowProps& props) {
    m_Data.Title = props.Title;
    m_Data.Width = props.Width;
    m_Data.Height = props.Height;

    LOG_INFO("Window", "Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    m_Window = SDL_CreateWindow(m_Data.Title.c_str(), m_Data.Width, m_Data.Height, window_flags);

    if (!m_Window) {
        LOG_FATAL("Window", "Failed to create SDL window: {0}", SDL_GetError());
    }
}

void PlatformSDLWindow::Shutdown() {
    if (m_Window) {
        SDL_DestroyWindow(m_Window);
        m_Window = nullptr;
    }
}

void PlatformSDLWindow::OnUpdate() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // 事件翻译工厂 (Event Translation Factory)
        switch (event.type) {
            case SDL_EVENT_QUIT: {
                WindowCloseEvent e;
                e.NativeEvent = &event;
                m_Data.EventCallback(e);
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED: {
                m_Data.Width = event.window.data1;
                m_Data.Height = event.window.data2;
                WindowResizeEvent e(m_Data.Width, m_Data.Height);
                e.NativeEvent = &event;
                m_Data.EventCallback(e);
                break;
            }
            case SDL_EVENT_KEY_DOWN: {
                KeyPressedEvent e(event.key.scancode, event.key.repeat);
                e.NativeEvent = &event;
                m_Data.EventCallback(e);
                break;
            }
            // ... 更多事件转换可在后续补全 ...
        }
    }
}

void PlatformSDLWindow::SetVSync(bool enabled) {
    m_Data.VSync = enabled;
}

bool PlatformSDLWindow::IsVSync() const {
    return m_Data.VSync;
}

} // namespace Prisma
