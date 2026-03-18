#pragma once

#include "../Export.h"
#include <string>
#include <functional>

namespace Prisma {

enum class EventType {
    None = 0,
    WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
    AppTick, AppUpdate, AppRender,
    KeyPressed, KeyReleased, KeyTyped,
    MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
};

enum EventCategory {
    None = 0,
    EventCategoryApplication    = 1 << 0,
    EventCategoryInput          = 1 << 1,
    EventCategoryKeyboard       = 1 << 2,
    EventCategoryMouse          = 1 << 3,
    EventCategoryMouseButton    = 1 << 4
};

#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
                                virtual EventType GetEventType() const override { return GetStaticType(); }\
                                virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

class ENGINE_API Event {
public:
    virtual ~Event() = default;

    bool Handled = false;
    void* NativeEvent = nullptr;

    virtual EventType GetEventType() const = 0;
    virtual const char* GetName() const = 0;
    virtual int GetCategoryFlags() const = 0;
    virtual std::string ToString() const { return GetName(); }

    bool IsInCategory(EventCategory category) {
        return GetCategoryFlags() & category;
    }
};

class ENGINE_API EventDispatcher {
public:
    EventDispatcher(Event& event) : m_Event(event) {}

    template<typename T, typename F>
    bool Dispatch(const F& func) {
        if (m_Event.GetEventType() == T::GetStaticType()) {
            m_Event.Handled |= func(static_cast<T&>(m_Event));
            return true;
        }
        return false;
    }

private:
    Event& m_Event;
};

// --- 具体事件定义 ---
// 添加了缺失的事件类以支持完整的输入处理，特别是为了与 ImGui 集成。

class ENGINE_API WindowResizeEvent : public Event {
public:
    WindowResizeEvent(unsigned int width, unsigned int height) : m_Width(width), m_Height(height) {}

    unsigned int GetWidth() const { return m_Width; }
    unsigned int GetHeight() const { return m_Height; }

    std::string ToString() const override {
        return "WindowResizeEvent: " + std::to_string(m_Width) + ", " + std::to_string(m_Height);
    }

    EVENT_CLASS_TYPE(WindowResize)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
private:
    unsigned int m_Width, m_Height;
};

class ENGINE_API WindowCloseEvent : public Event {
public:
    WindowCloseEvent() = default;
    EVENT_CLASS_TYPE(WindowClose)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class ENGINE_API KeyPressedEvent : public Event {
public:
    KeyPressedEvent(int keycode, bool repeat = false) : m_KeyCode(keycode), m_IsRepeat(repeat) {}

    int GetKeyCode() const { return m_KeyCode; }
    bool IsRepeat() const { return m_IsRepeat; }

    EVENT_CLASS_TYPE(KeyPressed)
    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
private:
    int m_KeyCode;
    bool m_IsRepeat;
};

class ENGINE_API KeyReleasedEvent : public Event {
public:
    KeyReleasedEvent(int keycode) : m_KeyCode(keycode) {}

    int GetKeyCode() const { return m_KeyCode; }

    EVENT_CLASS_TYPE(KeyReleased)
    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
private:
    int m_KeyCode;
};

class ENGINE_API KeyTypedEvent : public Event {
public:
    KeyTypedEvent(int keycode) : m_KeyCode(keycode) {}

    int GetKeyCode() const { return m_KeyCode; }

    EVENT_CLASS_TYPE(KeyTyped)
    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
private:
    int m_KeyCode;
};

class ENGINE_API MouseMovedEvent : public Event {
public:
    MouseMovedEvent(float x, float y) : m_MouseX(x), m_MouseY(y) {}

    float GetX() const { return m_MouseX; }
    float GetY() const { return m_MouseY; }

    std::string ToString() const override {
        return "MouseMovedEvent: " + std::to_string(m_MouseX) + ", " + std::to_string(m_MouseY);
    }

    EVENT_CLASS_TYPE(MouseMoved)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
private:
    float m_MouseX, m_MouseY;
};

class ENGINE_API MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(float xOffset, float yOffset) : m_XOffset(xOffset), m_YOffset(yOffset) {}

    float GetXOffset() const { return m_XOffset; }
    float GetYOffset() const { return m_YOffset; }

    std::string ToString() const override {
        return "MouseScrolledEvent: " + std::to_string(m_XOffset) + ", " + std::to_string(m_YOffset);
    }

    EVENT_CLASS_TYPE(MouseScrolled)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
private:
    float m_XOffset, m_YOffset;
};

class ENGINE_API MouseButtonEvent : public Event {
public:
    int GetMouseButton() const { return m_Button; }

    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)
protected:
    MouseButtonEvent(int button) : m_Button(button) {}
    int m_Button;
};

class ENGINE_API MouseButtonPressedEvent : public MouseButtonEvent {
public:
    MouseButtonPressedEvent(int button) : MouseButtonEvent(button) {}

    std::string ToString() const override {
        return "MouseButtonPressedEvent: " + std::to_string(m_Button);
    }

    EVENT_CLASS_TYPE(MouseButtonPressed)
};

class ENGINE_API MouseButtonReleasedEvent : public MouseButtonEvent {
public:
    MouseButtonReleasedEvent(int button) : MouseButtonEvent(button) {}

    std::string ToString() const override {
        return "MouseButtonReleasedEvent: " + std::to_string(m_Button);
    }

    EVENT_CLASS_TYPE(MouseButtonReleased)
};

} // namespace Prisma
