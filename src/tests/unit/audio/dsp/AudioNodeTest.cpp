#include <gtest/gtest.h>
#include <audio/dsp/AudioNode.h>

using namespace Prisma::Audio::DSP;

// ============================================================================
// 测试用具体节点
// ============================================================================

class TestNode : public AudioNode {
public:
    void Process(AudioBuffer& output, [[maybe_unused]] const AudioProcessContext& ctx) override {
        output.Clear();
    }
};

// ============================================================================
// 引脚管理
// ============================================================================

TEST(AudioNodeTest, AddInputPin) {
    TestNode node;
    AudioPin* pin = node.AddInputPin("audio_in");
    ASSERT_NE(pin, nullptr);
    EXPECT_EQ(pin->GetName(), "audio_in");
    EXPECT_EQ(pin->GetDirection(), PinDirection::Input);
    EXPECT_EQ(pin->GetOwner(), &node);
    EXPECT_FALSE(pin->IsConnected());
}

TEST(AudioNodeTest, AddOutputPin) {
    TestNode node;
    AudioPin* pin = node.AddOutputPin("audio_out");
    ASSERT_NE(pin, nullptr);
    EXPECT_EQ(pin->GetName(), "audio_out");
    EXPECT_EQ(pin->GetDirection(), PinDirection::Output);
    EXPECT_EQ(pin->GetOwner(), &node);
    EXPECT_FALSE(pin->IsConnected());
}

TEST(AudioNodeTest, GetInputPin) {
    TestNode node;
    node.AddInputPin("in1");
    node.AddInputPin("in2");

    AudioPin* found = node.GetInputPin("in1");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetName(), "in1");
    EXPECT_EQ(found->GetDirection(), PinDirection::Input);

    AudioPin* found2 = node.GetInputPin("in2");
    ASSERT_NE(found2, nullptr);
    EXPECT_EQ(found2->GetName(), "in2");

    // Non-existent pin
    EXPECT_EQ(node.GetInputPin("nonexistent"), nullptr);
}

TEST(AudioNodeTest, GetOutputPin) {
    TestNode node;
    node.AddOutputPin("out1");
    node.AddOutputPin("out2");

    AudioPin* found = node.GetOutputPin("out1");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetName(), "out1");
    EXPECT_EQ(found->GetDirection(), PinDirection::Output);

    EXPECT_EQ(node.GetOutputPin("nonexistent"), nullptr);
}

TEST(AudioNodeTest, MultiplePins) {
    TestNode node;
    AudioPin* in1  = node.AddInputPin("input1");
    AudioPin* in2  = node.AddInputPin("input2");
    AudioPin* out1 = node.AddOutputPin("output1");
    AudioPin* out2 = node.AddOutputPin("output2");

    ASSERT_NE(in1, nullptr);
    ASSERT_NE(in2, nullptr);
    ASSERT_NE(out1, nullptr);
    ASSERT_NE(out2, nullptr);

    EXPECT_NE(in1, in2);
    EXPECT_NE(out1, out2);
    EXPECT_EQ(node.GetInputPin("input1"), in1);
    EXPECT_EQ(node.GetInputPin("input2"), in2);
    EXPECT_EQ(node.GetOutputPin("output1"), out1);
    EXPECT_EQ(node.GetOutputPin("output2"), out2);
}

// ============================================================================
// 引脚连接
// ============================================================================

TEST(AudioNodeTest, ConnectPins) {
    TestNode src, dst;
    AudioPin* out = src.AddOutputPin("out");
    AudioPin* in  = dst.AddInputPin("in");

    ASSERT_NE(out, nullptr);
    ASSERT_NE(in, nullptr);

    out->Connect(in);
    EXPECT_TRUE(out->IsConnected());
    EXPECT_TRUE(in->IsConnected());
    EXPECT_EQ(out->GetConnection(), in);
    EXPECT_EQ(in->GetConnection(), out);
}

TEST(AudioNodeTest, DisconnectPins) {
    TestNode src, dst;
    AudioPin* out = src.AddOutputPin("out");
    AudioPin* in  = dst.AddInputPin("in");

    out->Connect(in);
    EXPECT_TRUE(out->IsConnected());

    out->Disconnect();
    EXPECT_FALSE(out->IsConnected());
    EXPECT_FALSE(in->IsConnected());
    EXPECT_EQ(out->GetConnection(), nullptr);
    EXPECT_EQ(in->GetConnection(), nullptr);
}

TEST(AudioNodeTest, SameDirectionNotAllowed) {
    TestNode a, b;
    AudioPin* out1 = a.AddOutputPin("out1");
    AudioPin* out2 = b.AddOutputPin("out2");

    out1->Connect(out2);
    EXPECT_FALSE(out1->IsConnected());
    EXPECT_FALSE(out2->IsConnected());
}

TEST(AudioNodeTest, ReconnectOverwrites) {
    TestNode src, dst1, dst2;
    AudioPin* out = src.AddOutputPin("out");
    AudioPin* in1 = dst1.AddInputPin("in1");
    AudioPin* in2 = dst2.AddInputPin("in2");

    out->Connect(in1);
    EXPECT_TRUE(out->IsConnected());
    EXPECT_TRUE(in1->IsConnected());

    // Reconnect to another input
    out->Connect(in2);
    EXPECT_FALSE(in1->IsConnected()); // old connection broken
    EXPECT_TRUE(out->IsConnected());
    EXPECT_EQ(out->GetConnection(), in2);
}

// ============================================================================
// 参数管理
// ============================================================================

TEST(AudioNodeTest, SetAndGetParameter) {
    TestNode node;
    node.SetParameter("gain", 0.5f);
    EXPECT_FLOAT_EQ(node.GetParameter("gain"), 0.5f);
}

TEST(AudioNodeTest, GetParameterDefault) {
    const TestNode node;
    EXPECT_FLOAT_EQ(node.GetParameter("nonexistent"), 0.0f);
}

TEST(AudioNodeTest, MultipleParameters) {
    TestNode node;
    node.SetParameter("gain", 0.5f);
    node.SetParameter("frequency", 440.0f);
    node.SetParameter("pan", -1.0f);

    EXPECT_FLOAT_EQ(node.GetParameter("gain"), 0.5f);
    EXPECT_FLOAT_EQ(node.GetParameter("frequency"), 440.0f);
    EXPECT_FLOAT_EQ(node.GetParameter("pan"), -1.0f);
}

TEST(AudioNodeTest, OverwriteParameter) {
    TestNode node;
    node.SetParameter("gain", 0.5f);
    node.SetParameter("gain", 1.0f);
    EXPECT_FLOAT_EQ(node.GetParameter("gain"), 1.0f);
}

// ============================================================================
// 节点名称
// ============================================================================

TEST(AudioNodeTest, DefaultName) {
    TestNode node;
    EXPECT_TRUE(node.GetName().empty());
}

TEST(AudioNodeTest, SetName) {
    TestNode node;
    node.SetName("oscillator");
    EXPECT_EQ(node.GetName(), "oscillator");
}

TEST(AudioNodeTest, OverwriteName) {
    TestNode node;
    node.SetName("old_name");
    node.SetName("new_name");
    EXPECT_EQ(node.GetName(), "new_name");
}

// ============================================================================
// 节点 ID
// ============================================================================

TEST(AudioNodeTest, DefaultId) {
    TestNode node;
    EXPECT_EQ(node.GetId(), 0); // 默认 ID 为 0，Graph 分配后才会改变
}

// ============================================================================
// 缓冲区挂载
// ============================================================================

TEST(AudioNodeTest, PinBuffer) {
    TestNode node;
    AudioPin* pin = node.AddOutputPin("out");
    ASSERT_NE(pin, nullptr);
    EXPECT_EQ(pin->GetBuffer(), nullptr);

    AudioBuffer buf(2, 64);
    pin->SetBuffer(&buf);
    ASSERT_NE(pin->GetBuffer(), nullptr);
    EXPECT_EQ(pin->GetBuffer(), &buf);

    pin->SetBuffer(nullptr);
    EXPECT_EQ(pin->GetBuffer(), nullptr);
}
