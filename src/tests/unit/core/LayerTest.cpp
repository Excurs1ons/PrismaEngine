#include <gtest/gtest.h>
#include "core/Layer.h"
#include "core/Event.h"

namespace Prisma {
namespace {

// ============================================================
// TestLayer - concrete Layer subclass tracking lifecycle calls
// ============================================================
class TestLayer : public Layer {
public:
    bool attached = false;
    bool detached = false;
    int updateCount = 0;
    int eventCount = 0;
    int renderCount = 0;
    int imguiRenderCount = 0;
    Event* lastEvent = nullptr;

    TestLayer(const std::string& name = "TestLayer")
        : Layer(name) {}

    void OnAttach() override { attached = true; }
    void OnDetach() override { detached = true; }
    void OnUpdate(Timestep /*ts*/) override { updateCount++; }
    void OnEvent(Event& e) override { eventCount++; lastEvent = &e; }
    void OnRender() override { renderCount++; }
    void OnImGuiRender() override { imguiRenderCount++; }
};

// Layer::OnAttach is called (manually in test)
TEST(LayerTest, OnAttachCalled) {
    TestLayer layer;
    layer.OnAttach();
    EXPECT_TRUE(layer.attached);
}

// Layer::OnDetach is called
TEST(LayerTest, OnDetachCalled) {
    TestLayer layer;
    layer.OnDetach();
    EXPECT_TRUE(layer.detached);
}

// Layer::OnUpdate increments counter
TEST(LayerTest, OnUpdateCalled) {
    TestLayer layer;
    EXPECT_EQ(layer.updateCount, 0);

    layer.OnUpdate(Timestep(1.0f));
    EXPECT_EQ(layer.updateCount, 1);

    layer.OnUpdate(Timestep(0.5f));
    EXPECT_EQ(layer.updateCount, 2);
}

// Layer::OnEvent receives events
TEST(LayerTest, OnEventCalled) {
    WindowResizeEvent event(800, 600);
    TestLayer layer;

    EXPECT_EQ(layer.eventCount, 0);
    EXPECT_EQ(layer.lastEvent, nullptr);

    layer.OnEvent(event);
    EXPECT_EQ(layer.eventCount, 1);
    EXPECT_NE(layer.lastEvent, nullptr);
    EXPECT_EQ(layer.lastEvent->GetEventType(), EventType::WindowResize);
}

// Layer::OnRender called
TEST(LayerTest, OnRenderCalled) {
    TestLayer layer;
    EXPECT_EQ(layer.renderCount, 0);
    layer.OnRender();
    EXPECT_EQ(layer.renderCount, 1);
}

// Layer::OnImGuiRender called
TEST(LayerTest, OnImGuiRenderCalled) {
    TestLayer layer;
    EXPECT_EQ(layer.imguiRenderCount, 0);
    layer.OnImGuiRender();
    EXPECT_EQ(layer.imguiRenderCount, 1);
}

// Layer constructor sets debug name
TEST(LayerTest, ConstructorSetsName) {
    TestLayer layer("MyCustomLayer");
    EXPECT_EQ(layer.GetName(), "MyCustomLayer");
}

// Default layer name
TEST(LayerTest, DefaultName) {
    Layer defaultLayer;
    EXPECT_EQ(defaultLayer.GetName(), "Layer");
}

// Layer virtual destructor works (polymorphic deletion)
TEST(LayerTest, VirtualDestructor) {
    Layer* layer = new TestLayer("DynamicLayer");
    EXPECT_EQ(layer->GetName(), "DynamicLayer");
    delete layer; // Should not leak or crash
}

// Event is passed by reference through layers
TEST(LayerTest, EventReferencePassthrough) {
    WindowResizeEvent event(1024, 768);
    TestLayer layer;

    layer.OnEvent(event);
    ASSERT_NE(layer.lastEvent, nullptr);
    EXPECT_EQ(static_cast<WindowResizeEvent*>(layer.lastEvent)->GetWidth(), 1024u);
    EXPECT_EQ(static_cast<WindowResizeEvent*>(layer.lastEvent)->GetHeight(), 768u);
}

} // namespace
} // namespace Prisma
