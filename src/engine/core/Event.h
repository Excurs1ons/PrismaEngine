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

// ... 其他事件(MouseMoved, MouseButton等)暂不罗列，结构一致 ...

} // namespace Prisma
