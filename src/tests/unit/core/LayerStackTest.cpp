#include <gtest/gtest.h>
#include "core/LayerStack.h"
#include "core/Event.h"
#include <vector>

namespace Prisma {
namespace {

// ============================================================
// TestLayer for LayerStack testing
// ============================================================
class StackTestLayer : public Layer {
public:
    using Layer::Layer;

    bool attached = false;
    bool detached = false;
    int eventCount = 0;
    std::vector<Event*> receivedEvents;

    void OnAttach() override { attached = true; }
    void OnDetach() override { detached = true; }
    void OnEvent(Event& e) override {
        eventCount++;
        receivedEvents.push_back(&e);
    }
};

// LayerStack pushes layers in order
TEST(LayerStackTest, PushLayerOrder) {
    LayerStack stack;

    auto* layer1 = new StackTestLayer("Layer1");
    auto* layer2 = new StackTestLayer("Layer2");

    stack.PushLayer(layer1);
    stack.PushLayer(layer2);

    // Layers should be in push order (layer1, layer2)
    auto it = stack.begin();
    EXPECT_EQ((*it)->GetName(), "Layer1");
    ++it;
    EXPECT_EQ((*it)->GetName(), "Layer2");

    // Don't delete - LayerStack owns them (destructor deletes)
}

// PushLayer calls OnAttach
TEST(LayerStackTest, PushLayerCallsOnAttach) {
    LayerStack stack;
    auto* layer = new StackTestLayer("AttachTest");

    EXPECT_FALSE(layer->attached);
    stack.PushLayer(layer);
    EXPECT_TRUE(layer->attached);
}

// PopLayer calls OnDetach
TEST(LayerStackTest, PopLayerCallsOnDetach) {
    LayerStack stack;
    auto* layer = new StackTestLayer("DetachTest");

    stack.PushLayer(layer);
    EXPECT_TRUE(layer->attached);

    stack.PopLayer(layer);
    EXPECT_TRUE(layer->detached);

    delete layer; // PopLayer does not delete
}

// Overlays are always after regular layers
TEST(LayerStackTest, OverlayAfterLayer) {
    LayerStack stack;

    auto* layer1 = new StackTestLayer("Layer1");
    auto* overlay1 = new StackTestLayer("Overlay1");
    auto* layer2 = new StackTestLayer("Layer2");
    auto* overlay2 = new StackTestLayer("Overlay2");

    stack.PushLayer(layer1);
    stack.PushOverlay(overlay1);
    stack.PushLayer(layer2);
    stack.PushOverlay(overlay2);

    // Expected order: Layer1, Layer2, Overlay1, Overlay2
    auto it = stack.begin();
    EXPECT_EQ((*it)->GetName(), "Layer1"); ++it;
    EXPECT_EQ((*it)->GetName(), "Layer2"); ++it;
    EXPECT_EQ((*it)->GetName(), "Overlay1"); ++it;
    EXPECT_EQ((*it)->GetName(), "Overlay2");
}

// PushOverlay with multiple inserts
TEST(LayerStackTest, MultipleOverlays) {
    LayerStack stack;

    auto* layer = new StackTestLayer("Main");
    auto* overlay1 = new StackTestLayer("Debug");
    auto* overlay2 = new StackTestLayer("ImGui");

    stack.PushLayer(layer);
    stack.PushOverlay(overlay1);
    stack.PushOverlay(overlay2);

    auto it = stack.begin();
    EXPECT_EQ((*it)->GetName(), "Main"); ++it;
    EXPECT_EQ((*it)->GetName(), "Debug"); ++it;
    EXPECT_EQ((*it)->GetName(), "ImGui");
}

// PopOverlay removes from the end
TEST(LayerStackTest, PopOverlay) {
    LayerStack stack;

    auto* layer = new StackTestLayer("Main");
    auto* overlay = new StackTestLayer("Overlay");

    stack.PushLayer(layer);
    stack.PushOverlay(overlay);

    EXPECT_EQ(std::distance(stack.begin(), stack.end()), 2);

    stack.PopOverlay(overlay);
    EXPECT_EQ(std::distance(stack.begin(), stack.end()), 1);
    EXPECT_EQ((*stack.begin())->GetName(), "Main");

    delete overlay; // PopOverlay does not delete
}

// PopLayer removes from the middle correctly
TEST(LayerStackTest, PopLayerFromMiddle) {
    LayerStack stack;

    auto* layer1 = new StackTestLayer("Layer1");
    auto* layer2 = new StackTestLayer("Layer2");
    auto* layer3 = new StackTestLayer("Layer3");
    auto* overlay = new StackTestLayer("Overlay");

    stack.PushLayer(layer1);
    stack.PushLayer(layer2);
    stack.PushLayer(layer3);
    stack.PushOverlay(overlay);

    stack.PopLayer(layer2);

    // Expected: Layer1, Layer3, Overlay
    auto it = stack.begin();
    EXPECT_EQ((*it)->GetName(), "Layer1"); ++it;
    EXPECT_EQ((*it)->GetName(), "Layer3"); ++it;
    EXPECT_EQ((*it)->GetName(), "Overlay");

    delete layer2;
}

// Iterator support (range-based for)
TEST(LayerStackTest, IteratorSupport) {
    LayerStack stack;
    auto* layer1 = new StackTestLayer("A");
    auto* layer2 = new StackTestLayer("B");
    stack.PushLayer(layer1);
    stack.PushLayer(layer2);

    std::vector<std::string> names;
    for (const auto& layer : stack) {
        names.push_back(layer->GetName());
    }

    ASSERT_EQ(names.size(), 2);
    EXPECT_EQ(names[0], "A");
    EXPECT_EQ(names[1], "B");
}

// Reverse iterator
TEST(LayerStackTest, ReverseIterator) {
    LayerStack stack;
    auto* layer1 = new StackTestLayer("First");
    auto* overlay = new StackTestLayer("Last");
    stack.PushLayer(layer1);
    stack.PushOverlay(overlay);

    auto it = stack.rbegin();
    EXPECT_EQ((*it)->GetName(), "Last"); ++it;
    EXPECT_EQ((*it)->GetName(), "First");
}

// Event propagation through layers
TEST(LayerStackTest, EventPropagation) {
    LayerStack stack;
    auto* layer = new StackTestLayer("Layer");
    auto* overlay = new StackTestLayer("Overlay");
    stack.PushLayer(layer);
    stack.PushOverlay(overlay);

    WindowResizeEvent event(800, 600);

    // Simulate event propagation (front to back)
    for (auto it = stack.begin(); it != stack.end(); ++it) {
        (*it)->OnEvent(event);
    }

    EXPECT_EQ(layer->eventCount, 1);
    EXPECT_EQ(overlay->eventCount, 1);
}

// Empty LayerStack
TEST(LayerStackTest, EmptyStack) {
    LayerStack stack;
    EXPECT_EQ(stack.begin(), stack.end());
}

// LayerStack destructor calls OnDetach on all layers
TEST(LayerStackTest, DestructorDetachesAll) {
    bool detachCalled = false;
    class DetachTrackerLayer : public Layer {
    public:
        bool* tracker;
        DetachTrackerLayer(const std::string& name, bool* t)
            : Layer(name), tracker(t) {}
        void OnDetach() override { *tracker = true; }
    };
    auto* layer = new DetachTrackerLayer("AutoDetach", &detachCalled);
    {
        LayerStack stack;
        stack.PushLayer(layer);
    }
    EXPECT_TRUE(detachCalled);
}

} // namespace
} // namespace Prisma
