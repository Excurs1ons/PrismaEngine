#include "Window.h"
#include "Logger.h"
#include "core/Event.h"
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>

namespace Prisma {


Window::~Window() {
    Shutdown();
}

void Window::Init(const WindowProps& props) {
    m_Data.Title = props.Title;
    m_Data.Width = props.Width;
    m_Data.Height = props.Height;

    LOG_INFO("Window", "正在创建窗口: {0} ({1}, {2})", props.Title, props.Width, props.Height);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    m_Window = SDL_CreateWindow(m_Data.Title.c_str(), m_Data.Width, m_Data.Height, window_flags);

    if (!m_Window) {
        LOG_FATAL("Window", "创建 SDL 窗口失败: {0}", SDL_GetError());
    }
}

void Window::Shutdown() {
    if (m_Window) {
        LOG_INFO("Window", "正在销毁 SDL 窗口...");
        SDL_DestroyWindow(m_Window);
        m_Window = nullptr;
        LOG_INFO("Window", "SDL 窗口已销毁");
    }
}

void Window::SetTitle(const std::string& title) {
    m_Data.Title = title;
    if (m_Window) {
        SDL_SetWindowTitle(m_Window, title.c_str());
    }
}

void Window::OnUpdate() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // 事件翻译工厂 (Event Translation Factory)
        // 完善了对鼠标、键盘、文本输入及窗口大小变化的事件处理，确保 ImGui 等系统能接收到必要的 NativeEvent。
        switch (event.type) {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            case SDL_EVENT_QUIT: {
                WindowCloseEvent e;
                e.NativeEvent = &event;
                m_Data.EventCallback(e);
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
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
            case SDL_EVENT_KEY_UP: {
                KeyReleasedEvent e(event.key.scancode);
                e.NativeEvent = &event;
                m_Data.EventCallback(e);
                break;
            }
            case SDL_EVENT_TEXT_INPUT: {
                // 对于 SDL3, text input 事件包含 UTF-8 字符串
                // 这里简化处理，通常 ImGui 会自己处理 NativeEvent
                KeyTypedEvent e(0); 
                e.NativeEvent = &event;
                m_Data.EventCallback(e);
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                MouseMovedEvent e(event.motion.x, event.motion.y);
                e.NativeEvent = &event;
                m_Data.EventCallback(e);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                MouseButtonPressedEvent e(event.button.button);
                e.NativeEvent = &event;
                m_Data.EventCallback(e);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                MouseButtonReleasedEvent e(event.button.button);
                e.NativeEvent = &event;
                m_Data.EventCallback(e);
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                MouseScrolledEvent e(event.wheel.x, event.wheel.y);
                e.NativeEvent = &event;
                m_Data.EventCallback(e);
                break;
            }
            // ... 更多事件转换可在后续补全 ...
            default: {
                // 即使没有对应的 Prisma Event，也将 NativeEvent 传给 App (主要是为了 ImGui)
                // 这里可以定义一个 GenericNativeEvent 或者直接用基类 Event
                // 但目前的 EditorLayer 只检查 event.NativeEvent 是否存在
                // 所以我们可以创建一个临时的 Event 实例
                struct NativeEventWrapper : public Event {
                    EventType GetEventType() const override { return EventType::None; }
                    const char* GetName() const override { return "NativeEvent"; }
                    int GetCategoryFlags() const override { return 0; }
                } e;
                e.NativeEvent = &event;
                m_Data.EventCallback(e);
                break;
            }
        }
    }
}

void Window::SetVSync(bool enabled) {
    m_Data.VSync = enabled;
}

bool Window::IsVSync() const {
    return m_Data.VSync;
}

std::unique_ptr<Window> Window::Create(const WindowProps& props) {
    // 基础 Window 类现在直接持有 SDL 句柄并处理事件。
    // 在 Prune 分支中，我们不再需要多层派生，直接实例化即可。
    class SDLWindowImpl : public Window {
    public:
        SDLWindowImpl(const WindowProps& props) {
            Init(props);
        }
    };

    return std::make_unique<SDLWindowImpl>(props);
}

} // namespace Prisma
