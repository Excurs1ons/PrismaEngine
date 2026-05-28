#include <gtest/gtest.h>
#include <audio/dsp/AudioBuffer.h>

using namespace Prisma::Audio::DSP;

// ============================================================================
// AudioBuffer — 构造与基本查询
// ============================================================================

TEST(AudioBufferTest, DefaultConstructor) {
    AudioBuffer buf;
    EXPECT_EQ(buf.GetChannels(), 0);
    EXPECT_EQ(buf.GetFrames(), 0);
    EXPECT_TRUE(buf.IsEmpty());
    EXPECT_EQ(buf.GetSampleCount(), 0);
}

TEST(AudioBufferTest, ParameterizedConstructor) {
    AudioBuffer buf(2, 512);
    EXPECT_EQ(buf.GetChannels(), 2);
    EXPECT_EQ(buf.GetFrames(), 512);
    EXPECT_FALSE(buf.IsEmpty());
    EXPECT_EQ(buf.GetSampleCount(), 2 * 512);
}

TEST(AudioBufferTest, MonoBuffer) {
    AudioBuffer buf(1, 256);
    EXPECT_EQ(buf.GetChannels(), 1);
    EXPECT_EQ(buf.GetFrames(), 256);
}

TEST(AudioBufferTest, LargeBuffer) {
    constexpr uint32_t kChannels = 8;
    constexpr uint32_t kFrames   = 1048576; // 1M frames per channel
    AudioBuffer buf(kChannels, kFrames);
    EXPECT_EQ(buf.GetChannels(), kChannels);
    EXPECT_EQ(buf.GetFrames(), kFrames);
    EXPECT_EQ(buf.GetSampleCount(), kChannels * kFrames);
    // Verify initial state is zeroed
    for (uint32_t ch = 0; ch < kChannels; ++ch) {
        const float* data = buf.GetChannel(ch);
        ASSERT_NE(data, nullptr);
        for (uint32_t i = 0; i < 64; ++i) {
            EXPECT_FLOAT_EQ(data[i], 0.0f);
        }
    }
}

// ============================================================================
// GetChannel — 声道访问
// ============================================================================

TEST(AudioBufferTest, GetChannelValid) {
    AudioBuffer buf(2, 128);
    float* ch0 = buf.GetChannel(0);
    float* ch1 = buf.GetChannel(1);
    ASSERT_NE(ch0, nullptr);
    ASSERT_NE(ch1, nullptr);
    // 验证声道间不重叠
    EXPECT_NE(ch0, ch1);
    EXPECT_EQ(ch1 - ch0, 128); // planar layout: ch0 then ch1
}

TEST(AudioBufferTest, GetChannelInvalid) {
    AudioBuffer buf(2, 64);
    EXPECT_EQ(buf.GetChannel(2), nullptr);
    EXPECT_EQ(buf.GetChannel(100), nullptr);
}

TEST(AudioBufferTest, GetChannelConst) {
    const AudioBuffer buf(2, 64);
    const float* ch0 = buf.GetChannel(0);
    const float* ch1 = buf.GetChannel(1);
    ASSERT_NE(ch0, nullptr);
    ASSERT_NE(ch1, nullptr);
    EXPECT_NE(ch0, ch1);
}

TEST(AudioBufferTest, ChannelDataIndependence) {
    AudioBuffer buf(2, 32);
    // Write ch0 = 1.0, ch1 = 2.0
    float* ch0 = buf.GetChannel(0);
    float* ch1 = buf.GetChannel(1);
    for (uint32_t i = 0; i < 32; ++i) {
        ch0[i] = 1.0f;
        ch1[i] = 2.0f;
    }
    // Verify independence
    for (uint32_t i = 0; i < 32; ++i) {
        EXPECT_FLOAT_EQ(ch0[i], 1.0f);
        EXPECT_FLOAT_EQ(ch1[i], 2.0f);
    }
}

// ============================================================================
// Clear / Fill
// ============================================================================

TEST(AudioBufferTest, Clear) {
    AudioBuffer buf(2, 64);
    float* ch0 = buf.GetChannel(0);
    float* ch1 = buf.GetChannel(1);
    ch0[0] = 0.5f;
    ch1[0] = -0.5f;

    buf.Clear();
    for (uint32_t i = 0; i < 64; ++i) {
        EXPECT_FLOAT_EQ(ch0[i], 0.0f);
        EXPECT_FLOAT_EQ(ch1[i], 0.0f);
    }
}

TEST(AudioBufferTest, Fill) {
    AudioBuffer buf(2, 32);
    buf.Fill(0.75f);
    for (uint32_t ch = 0; ch < 2; ++ch) {
        const float* data = buf.GetChannel(ch);
        for (uint32_t i = 0; i < 32; ++i) {
            EXPECT_FLOAT_EQ(data[i], 0.75f);
        }
    }
}

// ============================================================================
// Resize
// ============================================================================

TEST(AudioBufferTest, Resize) {
    AudioBuffer buf(1, 64);
    EXPECT_EQ(buf.GetChannels(), 1);
    EXPECT_EQ(buf.GetFrames(), 64);

    buf.Resize(2, 128);
    EXPECT_EQ(buf.GetChannels(), 2);
    EXPECT_EQ(buf.GetFrames(), 128);
    EXPECT_EQ(buf.GetSampleCount(), 256);

    // After resize, data should be zeroed
    for (uint32_t ch = 0; ch < 2; ++ch) {
        const float* data = buf.GetChannel(ch);
        for (uint32_t i = 0; i < 128; ++i) {
            EXPECT_FLOAT_EQ(data[i], 0.0f);
        }
    }
}

TEST(AudioBufferTest, ResizeNoop) {
    AudioBuffer buf(2, 64);
    buf.Fill(0.5f);
    buf.Resize(2, 64); // same size — no change
    const float* ch0 = buf.GetChannel(0);
    EXPECT_FLOAT_EQ(ch0[0], 0.5f);
}

// ============================================================================
// CopyFrom
// ============================================================================

TEST(AudioBufferTest, CopyFromSameConfig) {
    AudioBuffer src(2, 64);
    AudioBuffer dst(2, 64);
    src.Fill(0.42f);
    dst.Fill(0.0f);

    EXPECT_TRUE(dst.CopyFrom(src));
    for (uint32_t ch = 0; ch < 2; ++ch) {
        const float* d = dst.GetChannel(ch);
        for (uint32_t i = 0; i < 64; ++i) {
            EXPECT_FLOAT_EQ(d[i], 0.42f);
        }
    }
}

TEST(AudioBufferTest, CopyFromDifferentConfig) {
    AudioBuffer src(2, 64);
    AudioBuffer dst(2, 128); // larger
    src.Fill(0.42f);
    dst.Fill(0.0f);

    EXPECT_FALSE(dst.CopyFrom(src)); // different sizes, should fail
    // dst should remain unchanged
    const float* data = dst.GetChannel(0);
    EXPECT_FLOAT_EQ(data[0], 0.0f);
}

TEST(AudioBufferTest, CopyFromChannel) {
    AudioBuffer src(2, 64);
    AudioBuffer dst(2, 64);

    float* srcCh0 = src.GetChannel(0);
    for (uint32_t i = 0; i < 64; ++i) srcCh0[i] = static_cast<float>(i);
    src.GetChannel(1)[0] = 99.0f;
    dst.Fill(0.0f);

    // Copy src channel 0 → dst channel 1
    EXPECT_TRUE(dst.CopyFrom(src, 1, 0));
    const float* dstCh1 = dst.GetChannel(1);
    for (uint32_t i = 0; i < 64; ++i) {
        EXPECT_FLOAT_EQ(dstCh1[i], static_cast<float>(i));
    }
    // Channel 0 should be unchanged (still 0)
    const float* dstCh0 = dst.GetChannel(0);
    for (uint32_t i = 0; i < 64; ++i) {
        EXPECT_FLOAT_EQ(dstCh0[i], 0.0f);
    }
}

TEST(AudioBufferTest, CopyFromInvalidChannel) {
    AudioBuffer src(1, 64);
    AudioBuffer dst(2, 64);
    EXPECT_FALSE(dst.CopyFrom(src, 5, 0)); // dstChannel out of range
    EXPECT_FALSE(dst.CopyFrom(src, 0, 5)); // srcChannel out of range
}

// ============================================================================
// Mix
// ============================================================================

TEST(AudioBufferTest, Mix) {
    AudioBuffer dst(2, 64);
    AudioBuffer src(2, 64);
    dst.Fill(1.0f);
    src.Fill(2.0f);

    EXPECT_TRUE(dst.Mix(src, 0.5f));
    // Expected: dst[i] = 1.0 + 2.0 * 0.5 = 2.0
    const float* data = dst.GetChannel(0);
    for (uint32_t i = 0; i < 64; ++i) {
        EXPECT_FLOAT_EQ(data[i], 2.0f);
    }
}

TEST(AudioBufferTest, MixChannel) {
    AudioBuffer dst(2, 64);
    AudioBuffer src(1, 64);
    dst.Fill(1.0f);
    src.Fill(3.0f);

    EXPECT_TRUE(dst.Mix(src, 0.5f, 0, 0)); // mix src ch0 → dst ch0
    const float* dstCh0 = dst.GetChannel(0);
    const float* dstCh1 = dst.GetChannel(1);
    for (uint32_t i = 0; i < 64; ++i) {
        EXPECT_FLOAT_EQ(dstCh0[i], 1.0f + 3.0f * 0.5f); // 2.5
        EXPECT_FLOAT_EQ(dstCh1[i], 1.0f);                // unchanged
    }
}

TEST(AudioBufferTest, MixConfigMismatch) {
    AudioBuffer dst(2, 64);
    AudioBuffer src(1, 64);
    EXPECT_FALSE(dst.Mix(src)); // different channel count
}

// ============================================================================
// ApplyGain
// ============================================================================

TEST(AudioBufferTest, ApplyGainAllChannels) {
    AudioBuffer buf(2, 32);
    buf.Fill(0.5f);
    buf.ApplyGain(2.0f);
    const float* data = buf.GetChannel(0);
    for (uint32_t i = 0; i < 32; ++i) {
        EXPECT_FLOAT_EQ(data[i], 1.0f);
    }
}

TEST(AudioBufferTest, ApplyGainSingleChannel) {
    AudioBuffer buf(2, 32);
    buf.Fill(1.0f);
    buf.ApplyGain(0.5f, 0); // only channel 0
    const float* ch0 = buf.GetChannel(0);
    const float* ch1 = buf.GetChannel(1);
    for (uint32_t i = 0; i < 32; ++i) {
        EXPECT_FLOAT_EQ(ch0[i], 0.5f);
        EXPECT_FLOAT_EQ(ch1[i], 1.0f);
    }
}

TEST(AudioBufferTest, ApplyGainInvalidChannel) {
    AudioBuffer buf(2, 32);
    buf.Fill(1.0f);
    buf.ApplyGain(0.5f, 5); // invalid channel — should be no-op
    const float* data = buf.GetChannel(0);
    EXPECT_FLOAT_EQ(data[0], 1.0f);
}

// ============================================================================
// FindPeak
// ============================================================================

TEST(AudioBufferTest, FindPeak) {
    AudioBuffer buf(2, 64);
    buf.Fill(0.5f);
    float* ch0 = buf.GetChannel(0);
    ch0[10] = 0.9f;
    EXPECT_FLOAT_EQ(buf.FindPeak(), 0.9f);
}

TEST(AudioBufferTest, FindPeakAbsValue) {
    AudioBuffer buf(1, 64);
    buf.GetChannel(0)[0] = -0.85f;
    EXPECT_FLOAT_EQ(buf.FindPeak(), 0.85f);
}

TEST(AudioBufferTest, FindPeakEmpty) {
    const AudioBuffer buf;
    EXPECT_FLOAT_EQ(buf.FindPeak(), 0.0f);
}

TEST(AudioBufferTest, FindPeakByChannel) {
    AudioBuffer buf(2, 64);
    buf.Fill(0.3f);
    buf.GetChannel(0)[5] = 0.7f;
    buf.GetChannel(1)[5] = 0.6f;
    EXPECT_FLOAT_EQ(buf.FindPeak(0), 0.7f);
    EXPECT_FLOAT_EQ(buf.FindPeak(1), 0.6f);
}

TEST(AudioBufferTest, FindPeakByChannelInvalid) {
    const AudioBuffer buf(2, 64);
    EXPECT_FLOAT_EQ(buf.FindPeak(5), 0.0f);
}

// ============================================================================
// ComputeRMS
// ============================================================================

TEST(AudioBufferTest, ComputeRMS) {
    AudioBuffer buf(1, 4);
    float* data = buf.GetChannel(0);
    data[0] = 1.0f;
    data[1] = -1.0f;
    data[2] = 1.0f;
    data[3] = -1.0f;
    // sumSq = 1+1+1+1 = 4, mean = 1.0, sqrt = 1.0
    EXPECT_FLOAT_EQ(buf.ComputeRMS(), 1.0f);
}

TEST(AudioBufferTest, ComputeRMSZero) {
    AudioBuffer buf(1, 64);
    EXPECT_FLOAT_EQ(buf.ComputeRMS(), 0.0f);
}

TEST(AudioBufferTest, ComputeRMSEmpty) {
    const AudioBuffer buf;
    EXPECT_FLOAT_EQ(buf.ComputeRMS(), 0.0f);
}

// ============================================================================
// Clamp
// ============================================================================

TEST(AudioBufferTest, Clamp) {
    AudioBuffer buf(1, 4);
    float* data = buf.GetChannel(0);
    data[0] = 1.5f;
    data[1] = -2.0f;
    data[2] = 0.3f;
    data[3] = -0.5f;
    buf.Clamp();
    EXPECT_FLOAT_EQ(data[0], 1.0f);
    EXPECT_FLOAT_EQ(data[1], -1.0f);
    EXPECT_FLOAT_EQ(data[2], 0.3f);
    EXPECT_FLOAT_EQ(data[3], -0.5f);
}

// ============================================================================
// Data / GetData
// ============================================================================

TEST(AudioBufferTest, GetData) {
    AudioBuffer buf(2, 16);
    float* raw = buf.GetData();
    ASSERT_NE(raw, nullptr);
    raw[0] = 42.0f;
    EXPECT_FLOAT_EQ(buf.GetChannel(0)[0], 42.0f);
}

TEST(AudioBufferTest, GetDataConst) {
    AudioBuffer buf(1, 16);
    buf.GetChannel(0)[0] = 3.14f;
    const auto& constBuf = buf;
    const float* raw = constBuf.GetData();
    EXPECT_FLOAT_EQ(raw[0], 3.14f);
}
