#include <gtest/gtest.h>
#include "core/Event.h"

namespace Prisma {
namespace {

// ============================================================
// Event type identification
// ============================================================

TEST(EventTest, EventTypeEnum) {
    WindowResizeEvent resize(800, 600);
    EXPECT_EQ(resize.GetEventType(), EventType::WindowResize);
    EXPECT_STREQ(resize.GetName(), "WindowResize");

    WindowCloseEvent close;
    EXPECT_EQ(close.GetEventType(), EventType::WindowClose);
    EXPECT_STREQ(close.GetName(), "WindowClose");
}

// Different event types are not equal
TEST(EventTest, DifferentTypesAreDistinct) {
    WindowResizeEvent resize(800, 600);
    WindowCloseEvent close;
    KeyPressedEvent keyPress(42, false);

    EXPECT_NE(resize.GetEventType(), close.GetEventType());
    EXPECT_NE(resize.GetEventType(), keyPress.GetEventType());
    EXPECT_NE(close.GetEventType(), keyPress.GetEventType());
}

// ============================================================
// Event category flags
// ============================================================

TEST(EventTest, CategoryFlags) {
    WindowResizeEvent resize(800, 600);
    EXPECT_TRUE(resize.IsInCategory(EventCategory::EventCategoryApplication));
    EXPECT_FALSE(resize.IsInCategory(EventCategory::EventCategoryInput));

    KeyPressedEvent key(42, false);
    EXPECT_TRUE(key.IsInCategory(EventCategory::EventCategoryKeyboard));
    EXPECT_TRUE(key.IsInCategory(EventCategory::EventCategoryInput));

    MouseMovedEvent mouse(100.0f, 200.0f);
    EXPECT_TRUE(mouse.IsInCategory(EventCategory::EventCategoryMouse));
    EXPECT_TRUE(mouse.IsInCategory(EventCategory::EventCategoryInput));
}

// ============================================================
// Event data accessors
// ============================================================

TEST(EventTest, WindowResizeData) {
    WindowResizeEvent ev(1920, 1080);
    EXPECT_EQ(ev.GetWidth(), 1920u);
    EXPECT_EQ(ev.GetHeight(), 1080u);
}

TEST(EventTest, KeyPressedData) {
    KeyPressedEvent ev(32, true);
    EXPECT_EQ(ev.GetKeyCode(), 32);
    EXPECT_TRUE(ev.IsRepeat());

    KeyPressedEvent ev2(65, false);
    EXPECT_EQ(ev2.GetKeyCode(), 65);
    EXPECT_FALSE(ev2.IsRepeat());
}

TEST(EventTest, KeyReleasedData) {
    KeyReleasedEvent ev(27);
    EXPECT_EQ(ev.GetKeyCode(), 27);
}

TEST(EventTest, MouseMovedData) {
    MouseMovedEvent ev(123.5f, 456.7f);
    EXPECT_FLOAT_EQ(ev.GetX(), 123.5f);
    EXPECT_FLOAT_EQ(ev.GetY(), 456.7f);
}

TEST(EventTest, MouseScrolledData) {
    MouseScrolledEvent ev(1.0f, -2.0f);
    EXPECT_FLOAT_EQ(ev.GetXOffset(), 1.0f);
    EXPECT_FLOAT_EQ(ev.GetYOffset(), -2.0f);
}

TEST(EventTest, MouseButtonPressedData) {
    MouseButtonPressedEvent ev(1);
    EXPECT_EQ(ev.GetMouseButton(), 1);
}

TEST(EventTest, MouseButtonReleasedData) {
    MouseButtonReleasedEvent ev(2);
    EXPECT_EQ(ev.GetMouseButton(), 2);
}

// ============================================================
// EventToString
// ============================================================

TEST(EventTest, ToStringFormat) {
    WindowResizeEvent ev(800, 600);
    std::string str = ev.ToString();
    EXPECT_NE(str.find("800"), std::string::npos);
    EXPECT_NE(str.find("600"), std::string::npos);
}

// ============================================================
// EventDispatcher
// ============================================================

TEST(EventTest, DispatcherRoutesCorrectType) {
    WindowResizeEvent event(800, 600);
    EventDispatcher dispatcher(event);

    bool resizeHandled = false;
    bool keyHandled = false;

    dispatcher.Dispatch<WindowResizeEvent>([&](WindowResizeEvent&) {
        resizeHandled = true;
        return true; // handled
    });

    dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent&) {
        keyHandled = true;
        return true;
    });

    EXPECT_TRUE(resizeHandled);
    EXPECT_FALSE(keyHandled); // Wrong type, should not dispatch
}

// Handled flag propagation
TEST(EventTest, HandledFlagSetByDispatcher) {
    WindowResizeEvent event(800, 600);
    EventDispatcher dispatcher(event);

    EXPECT_FALSE(event.Handled);

    dispatcher.Dispatch<WindowResizeEvent>([&](WindowResizeEvent&) {
        return true; // Mark as handled
    });

    EXPECT_TRUE(event.Handled);
}

// Dispatcher does not set Handled if handler returns false
TEST(EventTest, DispatcherDoesNotSetHandledWhenFalse) {
    WindowResizeEvent event(800, 600);
    EventDispatcher dispatcher(event);

    dispatcher.Dispatch<WindowResizeEvent>([&](WindowResizeEvent&) {
        return false; // Not handled
    });

    EXPECT_FALSE(event.Handled);
}

// Multiple dispatches on same event accumulate handled flag
TEST(EventTest, MultipleDispatchesAccumulate) {
    WindowResizeEvent event(800, 600);
    EventDispatcher dispatcher(event);

    dispatcher.Dispatch<WindowResizeEvent>([&](WindowResizeEvent&) {
        return false; // First handler doesn't handle
    });

    EXPECT_FALSE(event.Handled);

    dispatcher.Dispatch<WindowResizeEvent>([&](WindowResizeEvent&) {
        return true; // Second handler does handle
    });

    EXPECT_TRUE(event.Handled);
}

// Event has default NativeEvent of nullptr
TEST(EventTest, NativeEventDefaultNull) {
    WindowResizeEvent ev(100, 200);
    EXPECT_EQ(ev.NativeEvent, nullptr);
}

} // namespace
} // namespace Prisma
