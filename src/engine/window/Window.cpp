#include "Window.h"
#include "Logger.h"
#include "core/Event.h"
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>

namespace Prisma {

Window::~Window() { Shutdown(); }

void Window::Init(const WindowProps& props) {
    m_Data.Title = props.Title; m_Data.Width = props.Width; m_Data.Height = props.Height;
    LOG_INFO("Window", "正在创建窗口: {0} ({1}, {2})", props.Title, props.Width, props.Height);
    SDL_WindowFlags f = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    m_Window = SDL_CreateWindow(m_Data.Title.c_str(), m_Data.Width, m_Data.Height, f);
    if (!m_Window) LOG_FATAL("Window", "创建 SDL 窗口失败: {0}", SDL_GetError());

    // 禁用高频鼠标移动事件，改用主动轮询，防止事件风暴
    SDL_SetEventEnabled(SDL_EVENT_MOUSE_MOTION, false);
}

void Window::Shutdown() { if (m_Window) { SDL_DestroyWindow(m_Window); m_Window = nullptr; } }
void Window::SetTitle(const std::string& title) { m_Data.Title = title; if (m_Window) SDL_SetWindowTitle(m_Window, title.c_str()); }

void Window::OnUpdate() {
    SDL_Event event;
    static float lastX = -1.0f, lastY = -1.0f;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            case SDL_EVENT_QUIT: { WindowCloseEvent e; e.NativeEvent = &event; m_Data.EventCallback(e); break; }
            case SDL_EVENT_WINDOW_RESIZED: {
                m_Data.Width = event.window.data1; m_Data.Height = event.window.data2;
                WindowResizeEvent e(m_Data.Width, m_Data.Height); e.NativeEvent = &event; m_Data.EventCallback(e); break;
            }
            case SDL_EVENT_KEY_DOWN: { KeyPressedEvent e(event.key.scancode, event.key.repeat); e.NativeEvent = &event; m_Data.EventCallback(e); break; }
            case SDL_EVENT_KEY_UP: { KeyReleasedEvent e(event.key.scancode); e.NativeEvent = &event; m_Data.EventCallback(e); break; }
            case SDL_EVENT_MOUSE_BUTTON_DOWN: { MouseButtonPressedEvent e(event.button.button); e.NativeEvent = &event; m_Data.EventCallback(e); break; }
            case SDL_EVENT_MOUSE_BUTTON_UP: { MouseButtonReleasedEvent e(event.button.button); e.NativeEvent = &event; m_Data.EventCallback(e); break; }
            case SDL_EVENT_MOUSE_WHEEL: { MouseScrolledEvent e(event.wheel.x, event.wheel.y); e.NativeEvent = &event; m_Data.EventCallback(e); break; }
            default: break;
        }
    }
    float curX, curY;
    SDL_GetMouseState(&curX, &curY);
    if (curX != lastX || curY != lastY) {
        lastX = curX; lastY = curY;
        MouseMovedEvent e(curX, curY); e.NativeEvent = nullptr;
        m_Data.EventCallback(e);
    }
}

void Window::SetVSync(bool enabled) { m_Data.VSync = enabled; }
bool Window::IsVSync() const { return m_Data.VSync; }

uint32_t Window::GetWidth() const { return m_Data.Width; }
uint32_t Window::GetHeight() const { return m_Data.Height; }

void Window::SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }
void* Window::GetNativeWindow() const { return m_Window; }

std::unique_ptr<Window> Window::Create(const WindowProps& props) {
    class SDLWindowImpl : public Window { public: SDLWindowImpl(const WindowProps& props) { Init(props); } };
    return std::make_unique<SDLWindowImpl>(props);
}

} // namespace Prisma
